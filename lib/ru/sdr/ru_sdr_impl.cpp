/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_sdr_impl.h"

using namespace ocudu;

ru_sdr_impl::ru_sdr_impl(const ru_sdr_impl_config& config, const ru_sdr_impl_dependencies& dependencies) :
  are_metrics_enabled(config.are_metrics_enabled),
  radio_metrics_collector(),
  radio_event_logger(dependencies.radio_logger),
  radio_event_dispatcher({&radio_metrics_collector, &radio_event_logger}),
  metrics_collector(radio_metrics_collector),
  radio_unit_controller(config.srate_MHz, config.start_time)
{
}

void ru_sdr_impl::set_ru_sectors(std::vector<std::unique_ptr<ru_sector_sdr_impl>> ru_sectors)
{
  sectors = std::move(ru_sectors);

  ocudu_assert(!sectors.empty(), "SDR Radio Unit received an empty list of sectors");

  radio_unit_controller.set_sector_controllers([this]() -> std::vector<ru_sector_controller_sdr_impl*> {
    std::vector<ru_sector_controller_sdr_impl*> output;
    sectors.reserve(sectors.size());
    for (auto& sector : sectors) {
      output.push_back(&sector->get_ru_sector_controller());
    }
    return output;
  }());

  metrics_collector.set_lower_phy_sectors([this]() -> std::vector<lower_phy_sector_metrics_collector*> {
    std::vector<lower_phy_sector_metrics_collector*> collectors;
    collectors.reserve(sectors.size());
    for (auto& sector : sectors) {
      collectors.push_back(&sector->get_metrics_collector());
    }
    return collectors;
  }());
}
