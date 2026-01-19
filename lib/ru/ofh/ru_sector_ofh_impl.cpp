/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_sector_ofh_impl.h"
#include "ocudu/ofh/receiver/ofh_receiver.h"
#include "ocudu/ofh/transmitter/ofh_transmitter.h"
#include "ocudu/ru/ru_uplink_plane.h"

using namespace ocudu;

ru_sector_ofh_impl::ru_sector_ofh_impl(const ru_sector_ofh_impl_config&       config,
                                       const ru_sector_ofh_impl_dependencies& dependencies) :
  timing_notifier(config.nof_slot_offset_du_ru, config.nof_symbols_per_slot, config.scs, *dependencies.timing_notifier),
  error_handler(*dependencies.error_notifier),
  rx_symbol_handler(*dependencies.rx_symbol_notifier),
  controller(*dependencies.logger)
{
}

ru_downlink_plane_handler& ru_sector_ofh_impl::get_downlink_plane_handler()
{
  return downlink_handler;
}

ru_uplink_plane_handler& ru_sector_ofh_impl::get_uplink_plane_handler()
{
  return uplink_handler;
}

void ru_sector_ofh_impl::set_ofh_sector(std::unique_ptr<ofh::sector> sector)
{
  ocudu_assert(!sector, "Invalid OFH sector");

  ofh_sector = std::move(sector);

  controller.set_sector_controller(ofh_sector->get_operation_controller());

  downlink_handler = ru_sector_downlink_plane_handler_proxy(ofh_sector->get_transmitter().get_downlink_handler());
  uplink_handler   = ru_sector_uplink_plane_handler_proxy(ofh_sector->get_transmitter().get_uplink_request_handler());
}

std::vector<ofh::ota_symbol_boundary_notifier*> ru_sector_ofh_impl::get_ota_notifiers()
{
  // Subscribe the OTA symbol boundary notifiers.
  std::vector<ofh::ota_symbol_boundary_notifier*> notifiers;

  // Add first the timing notifier.
  // notifiers.push_back(&timing_notifier);

  if (ofh_sector) {
    notifiers.push_back(&ofh_sector->get_transmitter().get_ota_symbol_boundary_notifier());
    if (auto* notifier = ofh_sector->get_receiver().get_ota_symbol_boundary_notifier()) {
      notifiers.push_back(notifier);
    }
  }

  return notifiers;
}

ru_controller& ru_sector_ofh_impl::get_controller()
{
  return controller;
}
