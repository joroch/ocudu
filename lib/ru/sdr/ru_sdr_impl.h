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

#include "ru_controller_sdr_impl.h"
#include "ru_metrics_collector_sdr_impl.h"
#include "ru_radio_event_handler.h"
#include "ru_radio_metrics_collector.h"
#include "ru_sector_sdr_impl.h"
#include "ocudu/radio/radio_session.h"
#include "ocudu/ru/ru.h"
#include <memory>

namespace ocudu {

/// SDR based Radio Unit implementation configuration.
struct ru_sdr_impl_config {
  double                                               srate_MHz;
  std::optional<std::chrono::system_clock::time_point> start_time;
  bool                                                 are_metrics_enabled;
};

/// SDR based Radio Unit implementation dependencies.
struct ru_sdr_impl_dependencies {
  ocudulog::basic_logger& logger;
  ocudulog::basic_logger& radio_logger;
};

/// SDR Radio Unit implementation.
class ru_sdr_impl : public radio_unit
{
public:
  ru_sdr_impl(const ru_sdr_impl_config& config, const ru_sdr_impl_dependencies& dependencies);

  // See interface for documentation.
  ru_operation_controller& get_operation_controller() override { return radio_unit_controller; }

  // See interface for documentation.
  ru_metrics_collector* get_metrics_collector() override { return are_metrics_enabled ? &metrics_collector : nullptr; }

  radio_unit_sector* get_radio_unit_sector(unsigned sector_id) override
  {
    ocudu_assert(sector_id < sectors.size(), "Invalid sector id '{}", sector_id);
    return sectors[sector_id].get();
  }

  /// Returns the radio event notifier of this RU.
  radio_event_notifier& get_radio_event_notifier() { return radio_event_dispatcher; }

  /// Sets the radio to the given one for this RU.
  void set_radio(std::unique_ptr<radio_session> radio_ptr)
  {
    radio = std::move(radio_ptr);
    ocudu_assert(radio, "Invalid radio");
    radio_unit_controller.set_radio(*radio);
  }

  /// Sets the RU sectors.
  void set_ru_sectors(std::vector<std::unique_ptr<ru_sector_sdr_impl>> ru_sectors);

private:
  const bool                                       are_metrics_enabled;
  ru_radio_metrics_collector                       radio_metrics_collector;
  ru_radio_logger_event_handler                    radio_event_logger;
  ru_radio_event_dispatcher                        radio_event_dispatcher;
  std::unique_ptr<radio_session>                   radio;
  std::vector<std::unique_ptr<ru_sector_sdr_impl>> sectors;
  ru_metrics_collector_sdr_impl                    metrics_collector;
  ru_controller_sdr_impl                           radio_unit_controller;
};

} // namespace ocudu
