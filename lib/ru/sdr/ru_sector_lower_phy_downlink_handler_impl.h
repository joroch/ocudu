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

#include "ocudu/phy/support/shared_resource_grid.h"
#include "ocudu/ru/ru_downlink_plane.h"

namespace ocudu {

class lower_phy_downlink_handler;

/// Radio Unit sector to lower physical layer downlink handler implementation.
class ru_sector_lower_phy_downlink_handler_impl : public ru_downlink_plane_handler
{
public:
  ru_sector_lower_phy_downlink_handler_impl();

  explicit ru_sector_lower_phy_downlink_handler_impl(lower_phy_downlink_handler& handler_) : handler(&handler_) {}

  // See interface for documentation.
  void handle_dl_data(const resource_grid_context& context, const shared_resource_grid& grid) override;

private:
  lower_phy_downlink_handler* handler;
};

} // namespace ocudu
