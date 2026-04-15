// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/ran/slot_point_extended.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include "ocudu/support/executors/task_executor.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <vector>

namespace ocudu {
namespace fapi_adaptor {

class fapi_dummy_sector;

/// \brief Wall-clock driven slot timing loop for the dummy FAPI PHY.
///
/// Fires one slot_indication per slot at the correct wall-clock rate derived from the SCS.
/// The loop reschedules itself via task_executor::defer() — the same pattern as ru_dummy_impl.
///
/// A custom clock function can be injected at construction time for deterministic unit testing.
///
/// \note stop() is non-blocking. After calling stop(), callers must ensure that all pending
///       tasks enqueued on the executor have completed before destroying this object.
///       In the production path the executor's worker thread will drain naturally; in tests,
///       call executor.run_pending_tasks() after stop().
class fapi_dummy_timing_handler
{
public:
  /// Callable that returns the current slot count (already wrapped modulo nof_slots_in_all_hyper_sfns).
  /// If empty, a wall-clock implementation is used.
  using clock_fn = std::function<uint64_t()>;

  /// Constructs the timing handler.
  ///
  /// \param scs           Subcarrier spacing that determines the slot rate.
  /// \param executor      Single-threaded executor on which the loop runs.
  /// \param sectors       Raw pointers to the dummy sectors that receive on_new_slot() calls.
  /// \param get_slot_fn   Optional injectable clock; defaults to wall clock when empty.
  fapi_dummy_timing_handler(subcarrier_spacing              scs,
                             task_executor&                  executor,
                             std::vector<fapi_dummy_sector*> sectors,
                             clock_fn                        get_slot_fn = {});

  ~fapi_dummy_timing_handler();

  /// Initialises the current slot from the clock and starts the loop.
  void start();

  /// Sets the stop flag. Non-blocking — the loop will terminate after at most one more iteration.
  void stop();

private:
  /// Re-enqueues the loop task unless stop has been requested.
  void defer_loop();

  /// Body of the timing loop — called repeatedly via defer().
  void loop();

  static constexpr std::chrono::microseconds minimum_loop_time{10};

  task_executor&                  executor;
  std::vector<fapi_dummy_sector*> sectors;
  clock_fn                        get_current_slot_fn;
  std::atomic<bool>               stopped{true};
  std::chrono::microseconds       slot_duration;
  slot_point_extended             current_slot;
};

} // namespace fapi_adaptor
} // namespace ocudu
