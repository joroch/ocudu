/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/ocudulog/logger.h"
#include "ocudu/ran/pci.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/scheduler/result/scheduler_result_handler.h"

namespace ocudu {

class scheduler_result_logger : public scheduler_result_handler
{
public:
  explicit scheduler_result_logger(bool log_broadcast_, pci_t pci_);

  void on_scheduler_result(slot_point                sl,
                           const sched_result&       result,
                           std::chrono::microseconds slot_latency = std::chrono::microseconds{0}) override;

private:
  void log_debug(const sched_result& result, std::chrono::microseconds slot_latency);

  void log_info(const sched_result& result, std::chrono::microseconds slot_latency);

  ocudulog::basic_logger& logger;
  bool                    log_broadcast;
  bool                    enabled;
  const pci_t             pci;

  fmt::memory_buffer fmtbuf;
};

} // namespace ocudu
