/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_ofh_impl.h"
#include "ocudu/ofh/receiver/ofh_receiver.h"

using namespace ocudu;

ru_ofh_impl::ru_ofh_impl(ru_ofh_impl_dependencies&& dependencies) :
  sectors(std::move(dependencies.sectors)),
  ofh_timing_mngr(std::move(dependencies.timing_mngr)),
  operation_controller(*dependencies.logger,
                       ofh_timing_mngr->get_controller(),
                       [](span<std::unique_ptr<ru_sector_ofh_impl>> elems) {
                         std::vector<ru_operation_controller*> controllers;
                         for (const auto& elem : elems) {
                           controllers.push_back(&(elem->get_controller().get_operation_controller()));
                         }
                         return controllers;
                       }(sectors)),
  metrics_collector(ofh_timing_mngr->get_metrics_collector(), [](span<std::unique_ptr<ru_sector_ofh_impl>> sectors_) {
    std::vector<ofh::metrics_collector*> out;
    for (const auto& sector : sectors_) {
      if (auto* collector = sector->get_metrics_collector()) {
        out.emplace_back(collector);
      }
    }
    return out;
  }(sectors))
{
  ocudu_assert(ofh_timing_mngr, "Invalid Open Fronthaul timing manager");
  ocudu_assert(!sectors.empty(), "Radio Unit does not contain sectors");

  // Configure the OTA notifiers.
  std::vector<ofh::ota_symbol_boundary_notifier*> notifiers;

  // :TODO: only add the first cell slot indication notifier.
  notifiers.push_back(&sectors.front()->get_slot_indication_notifier());

  for (const auto& sector : sectors) {
    std::vector<ofh::ota_symbol_boundary_notifier*> sector_notifier = sector->get_ota_notifiers();
    for (auto* notifier : sector_notifier) {
      notifiers.push_back(notifier);
    }
  }
  ofh_timing_mngr->get_ota_symbol_boundary_notifier_manager().subscribe(notifiers);
}

ru_metrics_collector* ru_ofh_impl::get_metrics_collector()
{
  return metrics_collector.disabled() ? nullptr : &metrics_collector;
}
