/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_sector_lower_phy_uplink_request_handler_impl.h"
#include "ocudu/phy/lower/lower_phy_uplink_request_handler.h"

using namespace ocudu;

namespace {

/// Dummy uplink request handler implementation.
class lower_phy_uplink_request_handler_dummy : public lower_phy_uplink_request_handler
{
public:
  void request_prach_window(const prach_buffer_context& context, shared_prach_buffer buffer) override
  {
    report_error("Dummy uplink request handler cannot process prach request");
  }
  void request_uplink_slot(const resource_grid_context& context, const shared_resource_grid& grid) override
  {
    report_error("Dummy uplink request handler cannot process uplink slot request");
  }
};

} // namespace

static lower_phy_uplink_request_handler_dummy dummy_handler;

ru_sector_lower_phy_uplink_request_handler_impl::ru_sector_lower_phy_uplink_request_handler_impl() :
  handler(&dummy_handler)
{
}

void ru_sector_lower_phy_uplink_request_handler_impl::handle_prach_occasion(const prach_buffer_context& context,
                                                                            shared_prach_buffer         buffer)
{
  handler->request_prach_window(context, std::move(buffer));
}

void ru_sector_lower_phy_uplink_request_handler_impl::handle_new_uplink_slot(const resource_grid_context& context,
                                                                             const shared_resource_grid&  grid)
{
  handler->request_uplink_slot(context, grid);
}
