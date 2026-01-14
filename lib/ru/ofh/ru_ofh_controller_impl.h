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

#include "ocudu/adt/span.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/ofh/ofh_controller.h"
#include "ocudu/ru/ru_controller.h"
#include <memory>

namespace ocudu {

/// \brief RU controller implementation for the Open Fronthaul interface.
///
/// Manages the timing controller that is common to all sectors and the individual controller of each sector.
class ru_ofh_controller_impl : public ru_operation_controller
{
public:
  ru_ofh_controller_impl(ocudulog::basic_logger&               logger_,
                         ofh::operation_controller&            timing_controller_,
                         std::vector<ru_operation_controller*> sector_controllers_);

  // See the ru_operation_controller interface for documentation.
  void start() override;

  // See the ru_operation_controller interface for documentation.
  void stop() override;

private:
  ocudulog::basic_logger&               logger;
  ofh::operation_controller&            timing_controller;
  std::vector<ru_operation_controller*> sector_controllers;
};

} // namespace ocudu
