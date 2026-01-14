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

#include "ru_ofh_controller_impl.h"
#include "ru_ofh_metrics_collector_impl.h"
#include "ru_sector_ofh_impl.h"
#include "ocudu/ofh/timing/ofh_timing_manager.h"
#include "ocudu/ru/ru.h"

namespace ocudu {

/// Open Fronthaul implementation dependencies.
struct ru_ofh_impl_dependencies {
  ocudulog::basic_logger*                          logger;
  std::unique_ptr<ofh::timing_manager>             timing_mngr;
  std::vector<std::unique_ptr<ru_sector_ofh_impl>> sectors;
};

/// Open Fronthaul Radio Unit implementation.
class ru_ofh_impl : public radio_unit
{
public:
  explicit ru_ofh_impl(ru_ofh_impl_dependencies&& dependencies);

  // See interface for documentation.
  ru_operation_controller& get_operation_controller() override { return operation_controller; }

  // See interface for documentation.
  radio_unit_sector* get_radio_unit_sector(unsigned sector_id) override
  {
    ocudu_assert(sector_id < sectors.size(), "Invalid sector id '{}'");
    return sectors[sector_id].get();
  }

  // See interface for documentation.
  ru_metrics_collector* get_metrics_collector() override;

private:
  std::vector<std::unique_ptr<ru_sector_ofh_impl>> sectors;
  std::unique_ptr<ofh::timing_manager>             ofh_timing_mngr;
  ru_ofh_controller_impl                           operation_controller;
  ru_ofh_metrics_collector_impl                    metrics_collector;
};

} // namespace ocudu
