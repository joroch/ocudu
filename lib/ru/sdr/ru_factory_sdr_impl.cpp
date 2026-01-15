/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lower_phy/lower_phy_factory.h"
#include "ru_radio_event_handler.h"
#include "ru_sdr_impl.h"
#include "ru_sector_lower_phy_error_adapter.h"
#include "ocudu/radio/radio_factory.h"
#include "ocudu/ru/sdr/ru_sdr_configuration.h"
#include "ocudu/ru/sdr/ru_sdr_factory.h"

using namespace ocudu;

/// Creates a radio session with the given parameters.
static std::unique_ptr<radio_session> create_radio_session(task_executor&                    executor,
                                                           radio_event_notifier&             radio_handler,
                                                           const radio_configuration::radio& config,
                                                           const std::string&                device_driver)
{
  print_available_radio_factories();

  std::unique_ptr<radio_factory> factory = create_radio_factory(device_driver);
  if (!factory) {
    return nullptr;
  }

  if (!factory->get_configuration_validator().is_configuration_valid(config)) {
    report_error("Invalid radio configuration.");
  }

  return factory->create(config, executor, radio_handler);
}

std::unique_ptr<radio_unit> ocudu::create_sdr_ru(const ru_sdr_configuration& config,
                                                 const ru_sdr_dependencies&  dependencies)
{
  auto ru =
      std::make_unique<ru_sdr_impl>(ru_sdr_impl_config{.srate_MHz           = config.radio_cfg.sampling_rate_Hz * 1e-6,
                                                       .start_time          = config.start_time,
                                                       .are_metrics_enabled = config.are_metrics_enabled},
                                    ru_sdr_impl_dependencies{.logger = dependencies.sector_dependencies.front().logger,
                                                             .radio_logger = dependencies.rf_logger});

  auto radio = create_radio_session(
      dependencies.radio_exec, ru->get_radio_event_notifier(), config.radio_cfg, config.device_driver);
  report_error_if_not(radio, "Unable to create radio session.");

  radio_session& radio_ref = *radio;

  ru->set_radio(std::move(radio));

  std::vector<std::unique_ptr<ru_sector_sdr_impl>> ru_sectors;

  for (unsigned sector_id = 0, sector_end = config.lower_phy_config.size(); sector_id != sector_end; ++sector_id) {
    const lower_phy_configuration&    lower_phy_cfg  = config.lower_phy_config[sector_id];
    const ru_sdr_sector_dependencies& ru_sector_deps = dependencies.sector_dependencies[sector_id];

    // TODO: notifier per cell.
    auto& ru_sector = ru_sectors.emplace_back(std::make_unique<ru_sector_sdr_impl>(
        ru_sector_sdr_impl_dependencies{.radio             = radio_ref,
                                        .rx_symbol_handler = dependencies.symbol_notifier,
                                        .timing_handler    = dependencies.timing_notifier,
                                        .logger            = dependencies.sector_dependencies.front().logger,
                                        .error_notifier    = dependencies.error_notifier}));

    lower_phy_sector_dependencies lophy_sector_deps = {
        .logger               = ru_sector_deps.logger,
        .rx_task_executor     = ru_sector_deps.rx_task_executor,
        .tx_task_executor     = ru_sector_deps.tx_task_executor,
        .dl_task_executor     = ru_sector_deps.dl_task_executor,
        .ul_task_executor     = ru_sector_deps.ul_task_executor,
        .prach_async_executor = ru_sector_deps.prach_async_executor,
        .error_notifier       = ru_sector->get_error_notifier(),
        // Only the first sector is used to report timing events using the RU notifier.
        .timing_notifier    = sector_id ? nullptr : &ru_sector->get_timing_notifier(),
        .bb_gateway         = radio_ref.get_baseband_gateway(sector_id),
        .rx_symbol_notifier = ru_sector->get_rx_symbol_notifier()};

    ru_sector->set_lower_phy(create_lower_phy_sector(lower_phy_cfg, lophy_sector_deps));
  }

  // Add lower PHY sector dependencies.
  ru->set_ru_sectors(std::move(ru_sectors));

  return ru;
}
