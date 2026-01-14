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

#include "ru_dummy_metrics_collector.h"
#include "ru_dummy_sector.h"
#include "ocudu/adt/interval.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/phy/support/prach_buffer.h"
#include "ocudu/phy/support/prach_buffer_context.h"
#include "ocudu/phy/support/resource_grid.h"
#include "ocudu/phy/support/resource_grid_context.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/ru/dummy/ru_dummy_configuration.h"
#include "ocudu/ru/ru.h"
#include "ocudu/ru/ru_controller.h"
#include "ocudu/ru/ru_downlink_plane.h"
#include "ocudu/ru/ru_timing_notifier.h"
#include "ocudu/ru/ru_uplink_plane.h"
#include "ocudu/support/executors/task_executor.h"
#include "ocudu/support/ocudu_assert.h"
#include "ocudu/support/synchronization/stop_event.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>

namespace ocudu {

/// \brief Implements a dummy radio unit.
///
/// The dummy radio unit is a radio unit designed for performance and stability testing purposes. It does not connect to
/// any RF device.
///
/// It determines the timing from the system time.
class ru_dummy_impl : public radio_unit, private ru_operation_controller
{
public:
  /// Creates a dummy radio unit from its configuration.
  explicit ru_dummy_impl(const ru_dummy_configuration& config, ru_dummy_dependencies dependencies) noexcept;

  // See the radio_unit interface for documentation.
  ru_metrics_collector* get_metrics_collector() override { return are_metrics_enabled ? &metrics_collector : nullptr; }

  // See the radio_unit interface for documentation.
  ru_operation_controller& get_operation_controller() override { return *this; }

  // See the radio_unit interface for documentation.
  radio_unit_sector* get_radio_unit_sector(unsigned sector_id) override
  {
    ocudu_assert(sector_id < sectors.size(), "Invalid sector if '{}'", sector_id);
    return sectors[sector_id].get();
  }

private:
  /// Minimum loop time.
  const std::chrono::microseconds minimum_loop_time = std::chrono::microseconds(10);

  // See ru_controller for documentation.
  void start() override;

  // See ru_controller for documentation.
  void stop() override;

  /// Defer loop task if the RU is running.
  /// \remark A fatal error is triggered if the executor fails to defer the loop task.
  void defer_loop();

  /// Loop execution task.
  void loop();

  /// Flag that enables (or not) metrics.
  const bool are_metrics_enabled;
  /// Ru logger.
  ocudulog::basic_logger& logger;
  /// Internal executor.
  task_executor& executor;
  /// Radio Unit timing notifier.
  ru_timing_notifier& timing_notifier;
  /// Stop control.
  stop_event_source stop_control;
  /// Slot time in microseconds.
  std::chrono::microseconds slot_duration;
  /// Number of slots is notified in advance of the transmission time.
  const unsigned max_processing_delay_slots;
  /// Current slot.
  slot_point current_slot;
  /// Radio unit sectors.
  std::vector<std::unique_ptr<ru_dummy_sector>> sectors;
  /// RU dummy metrics collector.
  ru_dummy_metrics_collector metrics_collector;
};

} // namespace ocudu
