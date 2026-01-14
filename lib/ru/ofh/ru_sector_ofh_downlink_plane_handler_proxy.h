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

#include "ocudu/ru/ru_downlink_plane.h"

namespace ocudu {

class task_executor;
class shared_resource_grid;

namespace ofh {
class downlink_handler;
} // namespace ofh

/// This proxy implementation dispatches the requests to the corresponding OFH sector.
class ru_sector_downlink_plane_handler_proxy : public ru_downlink_plane_handler
{
public:
  ru_sector_downlink_plane_handler_proxy();

  explicit ru_sector_downlink_plane_handler_proxy(ofh::downlink_handler& ofh_dl_handler_) :
    ofh_dl_handler(&ofh_dl_handler_)
  {
  }

  // See interface for documentation.
  void handle_dl_data(const resource_grid_context& context, const shared_resource_grid& grid) override;

private:
  ofh::downlink_handler* ofh_dl_handler;
};

} // namespace ocudu
