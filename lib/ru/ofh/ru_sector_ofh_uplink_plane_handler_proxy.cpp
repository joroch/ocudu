/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_sector_ofh_uplink_plane_handler_proxy.h"
#include "ocudu/ofh/transmitter/ofh_uplink_request_handler.h"
#include "ocudu/phy/support/prach_buffer_context.h"
#include "ocudu/phy/support/shared_resource_grid.h"

using namespace ocudu;

namespace {

class ofh_uplink_handler_dummy : public ofh::uplink_request_handler
{
public:
  void handle_prach_occasion(const prach_buffer_context& context, shared_prach_buffer buffer) override
  {
    report_error("Dummy OFH uplink request handler cannot process PRACH occassion");
  }
  void handle_new_uplink_slot(const resource_grid_context& context, const shared_resource_grid& grid) override
  {
    report_error("Dummy OFH uplink request handler cannot process uplink slot");
  }
};

} // namespace

static ofh_uplink_handler_dummy dummy_handler;

ru_sector_uplink_plane_handler_proxy::ru_sector_uplink_plane_handler_proxy() : ofh_ul_handler(&dummy_handler) {}

void ru_sector_uplink_plane_handler_proxy::handle_prach_occasion(const prach_buffer_context& context,
                                                                 shared_prach_buffer         buffer)
{
  ofh_ul_handler->handle_prach_occasion(context, std::move(buffer));
}

void ru_sector_uplink_plane_handler_proxy::handle_new_uplink_slot(const resource_grid_context& context,
                                                                  const shared_resource_grid&  grid)
{
  ocudu_assert(grid, "Invalid grid.");
  ofh_ul_handler->handle_new_uplink_slot(context, grid);
}
