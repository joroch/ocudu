/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/ru/ofh/ru_ofh_factory.h"
#include "ru_ofh_impl.h"
#include "ru_sector_ofh_impl.h"
#include "ocudu/ofh/ofh_factories.h"
#include "ocudu/support/error_handling.h"

using namespace ocudu;

std::unique_ptr<radio_unit> ocudu::create_ofh_ru(const ru_ofh_configuration& config, ru_ofh_dependencies&& dependencies)
{
  report_fatal_error_if_not(dependencies.timing_notifier, "Invalid timing notifier");

  std::vector<std::unique_ptr<ru_sector_ofh_impl>> sectors;

  // Create sectors.
  for (unsigned i = 0, e = config.sector_configs.size(); i != e; ++i) {
    const auto& sector_cfg = config.sector_configs[i];

    ru_sector_ofh_impl_config ru_sector_config{
        .nof_slot_offset_du_ru = config.sector_configs.back().max_processing_delay_slots,
        .nof_symbols_per_slot  = get_nsymb_per_slot(config.sector_configs.back().cp),
        .scs                   = config.sector_configs.back().scs};

    // :TODO: pass different notifiers per sector.
    ru_sector_ofh_impl_dependencies ru_sector_dependencies{.logger             = dependencies.logger,
                                                           .timing_notifier    = dependencies.timing_notifier,
                                                           .error_notifier     = dependencies.error_notifier,
                                                           .rx_symbol_notifier = dependencies.rx_symbol_notifier};

    auto& ru_sector =
        sectors.emplace_back(std::make_unique<ru_sector_ofh_impl>(ru_sector_config, ru_sector_dependencies));

    report_fatal_error_if_not(sector_cfg.max_processing_delay_slots >= 1,
                              "max_processing_delay_slots option should be greater than or equal to 1");

    report_fatal_error_if_not(
        sector_cfg.ru_operating_bw >= sector_cfg.bw,
        "The RU operating bandwidth should be greater than or equal to the bandwidth of the cell");

    // Fill the notifier in the dependencies.
    dependencies.sector_dependencies[i].notifier     = &ru_sector->get_uplane_rx_symbol_notifier();
    dependencies.sector_dependencies[i].err_notifier = &ru_sector->get_error_notifier();

    // Create OFH sector.
    auto sector = ofh::create_ofh_sector(sector_cfg, std::move(dependencies.sector_dependencies[i]));
    report_fatal_error_if_not(sector, "Unable to create OFH sector");

    ru_sector->set_ofh_sector(std::move(sector));

    fmt::println("Initializing the Open Fronthaul Interface for sector#{}: ul_compr=[{},{}], dl_compr=[{},{}], "
                 "prach_compr=[{},{}], prach_cp_enabled={}{}",
                 i,
                 to_string(sector_cfg.ul_compression_params.type),
                 sector_cfg.ul_compression_params.data_width,
                 to_string(sector_cfg.dl_compression_params.type),
                 sector_cfg.dl_compression_params.data_width,
                 to_string(sector_cfg.prach_compression_params.type),
                 sector_cfg.prach_compression_params.data_width,
                 sector_cfg.is_prach_control_plane_enabled,
                 (sector_cfg.bw != sector_cfg.ru_operating_bw)
                     ? fmt::format(".\nOperating a {}MHz cell over a RU with instantaneous bandwidth of {}MHz",
                                   fmt::underlying(sector_cfg.bw),
                                   fmt::underlying(sector_cfg.ru_operating_bw))
                     : fmt::format(""));
  }

  // Prepare OFH controller configuration.
  ofh::controller_config controller_cfg;
  controller_cfg.cp                            = config.sector_configs.back().cp;
  controller_cfg.scs                           = config.sector_configs.back().scs;
  controller_cfg.gps_Alpha                     = config.gps_Alpha;
  controller_cfg.gps_Beta                      = config.gps_Beta;
  controller_cfg.enable_log_warnings_for_lates = config.sector_configs.back().enable_log_warnings_for_lates;

  // Create OFH timing controller.
  auto timing_mngr =
      ofh::create_ofh_timing_manager(controller_cfg, *dependencies.logger, *dependencies.rt_timing_executor);
  report_fatal_error_if_not(timing_mngr, "Unable to create OFH timing manager");

  return std::make_unique<ru_ofh_impl>(ru_ofh_impl_dependencies{
      .logger = dependencies.logger, .timing_mngr = std::move(timing_mngr), .sectors = std::move(sectors)});
}
