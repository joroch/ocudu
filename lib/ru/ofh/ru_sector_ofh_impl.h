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

#include "ru_ofh_controller_impl.h"
#include "ru_sector_ofh_controller_impl.h"
#include "ru_sector_ofh_downlink_plane_handler_proxy.h"
#include "ru_sector_ofh_error_handler_impl.h"
#include "ru_sector_ofh_rx_symbol_handler_impl.h"
#include "ru_sector_ofh_timing_notifier_impl.h"
#include "ru_sector_ofh_uplink_plane_handler_proxy.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/ofh/ofh_sector.h"
#include "ocudu/ru/ofh/ru_ofh_configuration.h"
#include "ocudu/ru/ru.h"
#include "ocudu/ru/ru_timing_notifier.h"
#include "ocudu/ru/ru_uplink_plane.h"

namespace ocudu {

/// Open Fronthaul implementation configuration.
struct ru_sector_ofh_impl_config {
  unsigned           nof_slot_offset_du_ru;
  unsigned           nof_symbols_per_slot;
  subcarrier_spacing scs;
};

/// Open Fronthaul implementation dependencies.
struct ru_sector_ofh_impl_dependencies {
  ocudulog::basic_logger*             logger;
  ru_timing_notifier*                 timing_notifier    = nullptr;
  ru_error_notifier*                  error_notifier     = nullptr;
  ru_uplink_plane_rx_symbol_notifier* rx_symbol_notifier = nullptr;
};

/// Open Fronthaul Radio Unit sector implementation.
class ru_sector_ofh_impl : public radio_unit_sector
{
public:
  ru_sector_ofh_impl(const ru_sector_ofh_impl_config& config, const ru_sector_ofh_impl_dependencies& dependencies);

  // See interface for documentation.
  ru_controller& get_controller() override;

  // See interface for documentation.
  ru_downlink_plane_handler& get_downlink_plane_handler() override;

  // See interface for documentation.
  ru_uplink_plane_handler& get_uplink_plane_handler() override;

  /// Returns the error notifier of this RU.
  ofh::error_notifier& get_error_notifier() { return error_handler; }

  /// Returns the Open Fronthaul received symbol notifier of this Radio Unit.
  ofh::uplane_rx_symbol_notifier& get_uplane_rx_symbol_notifier() { return rx_symbol_handler; }

  /// Sets the OFH sector managed by this RU sector.
  void set_ofh_sector(std::unique_ptr<ofh::sector> sector);

  /// Returns the OTA notifier used for the SLOT.indication.
  ofh::ota_symbol_boundary_notifier& get_slot_indication_notifier() { return timing_notifier; }

  /// Returns the OTA notifiers of this OFH Radio Unit sector.
  std::vector<ofh::ota_symbol_boundary_notifier*> get_ota_notifiers();

  /// Returns the OFH metrics collector.
  ofh::metrics_collector* get_metrics_collector() { return ofh_sector->get_metrics_collector(); }

private:
  ru_sector_ofh_timing_notifier_impl     timing_notifier;
  ru_sector_ofh_error_handler_impl       error_handler;
  ru_sector_ofh_rx_symbol_handler_impl   rx_symbol_handler;
  std::unique_ptr<ofh::sector>           ofh_sector;
  ru_sector_ofh_controller_impl          controller;
  ru_sector_downlink_plane_handler_proxy downlink_handler;
  ru_sector_uplink_plane_handler_proxy   uplink_handler;
};

} // namespace ocudu
