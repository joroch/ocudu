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

#include "lower_phy/lower_phy_sector.h"
#include "ru_sector_controller_sdr_impl.h"
#include "ru_sector_lower_phy_downlink_handler_impl.h"
#include "ru_sector_lower_phy_error_adapter.h"
#include "ru_sector_lower_phy_rx_symbol_adapter.h"
#include "ru_sector_lower_phy_timing_adapter.h"
#include "ru_sector_lower_phy_uplink_request_handler_impl.h"
#include "ocudu/ru/ru.h"
#include <memory>

namespace ocudu {

/// SDR based Radio Unit implementation dependencies.
struct ru_sector_sdr_impl_dependencies {
  radio_session&                      radio;
  ru_uplink_plane_rx_symbol_notifier& rx_symbol_handler;
  ru_timing_notifier&                 timing_handler;
  ocudulog::basic_logger&             logger;
  ru_error_notifier&                  error_notifier;
};

/// SDR Radio Unit sector implementation.
class ru_sector_sdr_impl : public radio_unit_sector
{
public:
  ru_sector_sdr_impl(const ru_sector_sdr_impl_dependencies& dependencies);

  // See interface for documentation.
  ru_controller& get_controller() override { return controller; }

  // See interface for documentation
  ru_downlink_plane_handler& get_downlink_plane_handler() override { return ru_downlink_hdlr; }

  // See interface for documentation.
  ru_uplink_plane_handler& get_uplink_plane_handler() override { return ru_uplink_request_hdlr; }

  /// Returns the lower PHY rx symbol notifier of this RU sector.
  lower_phy_rx_symbol_notifier& get_rx_symbol_notifier() { return rx_adapter; }

  /// Returns the lower PHY timing notifier of this RU sector.
  lower_phy_timing_notifier& get_timing_notifier() { return timing_adapter; }

  /// Returns the lower PHY error notifier of this RU sector.
  lower_phy_error_notifier& get_error_notifier() { return error_adapter; }

  /// Returns the lower PHY metrics collector of this RU sector.
  lower_phy_sector_metrics_collector& get_metrics_collector()
  {
    ocudu_assert(low_phy, "Invalid lower PHY sector");

    return low_phy->get_metrics_collector();
  }

  /// Sets the RU lower PHY.
  void set_lower_phy(std::unique_ptr<lower_phy_sector> sector);

  /// Returns the RU sector controller.
  ru_sector_controller_sdr_impl& get_ru_sector_controller() { return controller; }

private:
  radio_session&                                  radio;
  ru_sector_lower_phy_error_adapter               error_adapter;
  ru_sector_lower_phy_rx_symbol_adapter           rx_adapter;
  ru_sector_lower_phy_timing_adapter              timing_adapter;
  std::unique_ptr<lower_phy_sector>               low_phy;
  ru_sector_controller_sdr_impl                   controller;
  ru_sector_lower_phy_downlink_handler_impl       ru_downlink_hdlr;
  ru_sector_lower_phy_uplink_request_handler_impl ru_uplink_request_hdlr;
};

} // namespace ocudu
