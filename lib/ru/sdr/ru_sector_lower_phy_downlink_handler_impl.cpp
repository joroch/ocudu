/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_sector_lower_phy_downlink_handler_impl.h"
#include "ocudu/phy/lower/lower_phy_downlink_handler.h"
#include "ocudu/phy/support/shared_resource_grid.h"

using namespace ocudu;

namespace {

/// Dummy downlink handler implementation.
class lower_phy_downlink_handler_dummy : public lower_phy_downlink_handler
{
public:
  void handle_resource_grid(const resource_grid_context& context, const shared_resource_grid& grid) override
  {
    report_error("Dummy lower PHY downlink handler cannot process grid");
  }
};

} // namespace

static lower_phy_downlink_handler_dummy dummy_handler;

ru_sector_lower_phy_downlink_handler_impl::ru_sector_lower_phy_downlink_handler_impl() : handler(&dummy_handler) {}

void ru_sector_lower_phy_downlink_handler_impl::handle_dl_data(const resource_grid_context& context,
                                                               const shared_resource_grid&  grid)
{
  handler->handle_resource_grid(context, grid);
}
