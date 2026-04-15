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

  // If the wall clock has not advanced to the next slot yet, yield briefly.
  if (slot_count == current_slot.system_slot()) {
    OCUDU_RTSAN_SCOPED_DISABLER(scoped_disabler);
    std::this_thread::sleep_for(minimum_loop_time);
  }

  // Advance current_slot up to slot_count, firing slot indications along the way.
  while (slot_count != current_slot.system_slot()) {
    ++current_slot;
    for (auto* sector : sectors) {
      sector->on_new_slot(current_slot);
    }
  }

  defer_loop();
}
