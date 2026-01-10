/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/ran/slot_point.h"
#include <chrono>
#include <memory>

namespace ocudu {

struct cell_configuration;
struct sched_result;

/// Interface used to handle MAC scheduler results.
class scheduler_result_handler
{
public:
  virtual ~scheduler_result_handler() = default;

  /// \brief Handle a scheduler result for a specific cell and slot.
  /// \param[in] sl Slot that was scheduled.
  /// \param[in] result Scheduling result for this slot.
  /// \param[in] slot_latency Latency that it took for the scheduler to make the decision for the slot.
  virtual void on_scheduler_result(slot_point                sl,
                                   const sched_result&       result,
                                   std::chrono::microseconds slot_latency = std::chrono::microseconds{0}) = 0;
};

/// Scheduler result handler factory interface.
std::unique_ptr<scheduler_result_handler> create_scheduler_result_handler(const cell_configuration& cell_cfg);

} // namespace ocudu
