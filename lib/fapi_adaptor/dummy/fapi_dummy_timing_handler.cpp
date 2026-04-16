// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "fapi_dummy_timing_handler.h"
#include "fapi_dummy_sector.h"
#include "ocudu/support/math/math_utils.h"
#include "ocudu/support/rtsan.h"
#include <chrono>
#include <thread>

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

/// Compute the current slot number from the system wall clock.
static uint64_t wall_clock_slot(std::chrono::microseconds slot_duration, uint64_t nof_slots)
{
  auto t = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch());
  return (t / slot_duration) % nof_slots;
}

fapi_dummy_timing_handler::fapi_dummy_timing_handler(subcarrier_spacing              scs,
                                                      task_executor&                  executor_,
                                                      std::vector<fapi_dummy_sector*> sectors_,
                                                      clock_fn                        get_slot_fn) :
  executor(executor_),
  sectors(std::move(sectors_)),
  slot_duration(1000U / pow2(to_numerology_value(scs))),
  current_slot(scs, 0)
{
  if (get_slot_fn) {
    get_current_slot_fn = std::move(get_slot_fn);
  } else {
    get_current_slot_fn = [this]() {
      return wall_clock_slot(slot_duration, current_slot.nof_slots_in_all_hyper_sfns());
    };
  }
}

fapi_dummy_timing_handler::~fapi_dummy_timing_handler()
{
  stop();
}

void fapi_dummy_timing_handler::start()
{
  uint64_t initial = get_current_slot_fn();
  current_slot     = slot_point_extended(current_slot.scs(), static_cast<uint32_t>(initial));
  stopped.store(false, std::memory_order_relaxed);
  defer_loop();
}

void fapi_dummy_timing_handler::stop()
{
  stopped.store(true, std::memory_order_relaxed);
}

void fapi_dummy_timing_handler::defer_loop()
{
  if (stopped.load(std::memory_order_relaxed)) {
    return;
  }
  // Ignore enqueue failures — if the executor is shutting down we simply stop looping.
  (void)executor.defer([this]() noexcept { loop(); });
}

void fapi_dummy_timing_handler::loop()
{
  // Re-check the stop flag at the entry of every iteration so the loop terminates promptly.
  if (stopped.load(std::memory_order_relaxed)) {
    return;
  }

  uint64_t slot_count = get_current_slot_fn();

  if (slot_count == current_slot.system_slot()) {
    // Wall clock has not advanced yet — yield briefly.
    OCUDU_RTSAN_SCOPED_DISABLER(scoped_disabler);
    std::this_thread::sleep_for(minimum_loop_time);
  } else {
    // Advance by exactly ONE slot per iteration and fire it via try_on_new_slot().
    // Firing one slot at a time prevents burst-flooding the MAC's SPSC slot_ind queue
    // (capacity 4) when catching up after a slow slot (e.g. during RA procedures).
    // try_on_new_slot() returns false when the MAC's in-flight credit is exhausted;
    // in that case we back off without advancing current_slot so we retry next loop.
    slot_point_extended next_slot = current_slot;
    ++next_slot;

    bool all_accepted = true;
    for (auto* sector : sectors) {
      if (!sector->try_on_new_slot(next_slot)) {
        all_accepted = false;
        break;
      }
    }

    if (all_accepted) {
      current_slot = next_slot;
    } else {
      OCUDU_RTSAN_SCOPED_DISABLER(scoped_disabler);
      std::this_thread::sleep_for(minimum_loop_time);
    }
  }

  defer_loop();
}
