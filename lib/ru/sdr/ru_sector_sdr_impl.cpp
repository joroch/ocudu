/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_sector_sdr_impl.h"

using namespace ocudu;

ru_sector_sdr_impl::ru_sector_sdr_impl(const ru_sector_sdr_impl_dependencies& dependencies) :
  radio(dependencies.radio),
  error_adapter(dependencies.logger, dependencies.error_notifier),
  rx_adapter(dependencies.rx_symbol_handler),
  timing_adapter(dependencies.timing_handler)
{
}

void ru_sector_sdr_impl::set_lower_phy(std::unique_ptr<lower_phy_sector> sector)
{
  ocudu_assert(sector, "Invalid lower PHY sector of the SDR Radio Unit");

  low_phy = std::move(sector);

  controller.set_radio(radio);
  controller.set_lower_phy_sector(*low_phy);
  ru_downlink_hdlr       = ru_sector_lower_phy_downlink_handler_impl(low_phy->get_downlink_handler());
  ru_uplink_request_hdlr = ru_sector_lower_phy_uplink_request_handler_impl(low_phy->get_uplink_request_handler());
}
