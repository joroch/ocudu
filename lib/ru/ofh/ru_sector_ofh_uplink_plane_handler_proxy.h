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

#include "ocudu/ru/ru_uplink_plane.h"

namespace ocudu {

class task_executor;
class shared_resource_grid;

namespace ofh {
class uplink_request_handler;
} // namespace ofh

/// This proxy implementation dispatches the requests to the corresponding OFH sector.
class ru_sector_uplink_plane_handler_proxy : public ru_uplink_plane_handler
{
public:
  ru_sector_uplink_plane_handler_proxy();

  explicit ru_sector_uplink_plane_handler_proxy(ofh::uplink_request_handler& ofh_ul_handler_) :
    ofh_ul_handler(&ofh_ul_handler_)
  {
  }

  // See interface for documentation.
  void handle_prach_occasion(const prach_buffer_context& context, shared_prach_buffer buffer) override;

  // See interface for documentation.
  void handle_new_uplink_slot(const resource_grid_context& context, const shared_resource_grid& grid) override;

private:
  ofh::uplink_request_handler* ofh_ul_handler;
};

} // namespace ocudu
