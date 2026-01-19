/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_sector_ofh_downlink_plane_handler_proxy.h"
#include "ocudu/ofh/transmitter/ofh_downlink_handler.h"

using namespace ocudu;

namespace {

class ofh_downlink_handler_dummy : public ofh::downlink_handler
{
public:
  void handle_dl_data(const resource_grid_context& context, const shared_resource_grid& grid) override
  {
    report_error("Dummy OFH downlink handler cannot process downlink data");
  }
};

} // namespace

static ofh_downlink_handler_dummy dummy_handler;

ru_sector_downlink_plane_handler_proxy::ru_sector_downlink_plane_handler_proxy() : ofh_dl_handler(&dummy_handler) {}

void ru_sector_downlink_plane_handler_proxy::handle_dl_data(const resource_grid_context& context,
                                                            const shared_resource_grid&  grid)
{
  ofh_dl_handler->handle_dl_data(context, grid);
}
