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

#include "ocudu/ocudulog/logger.h"
#include "ocudu/ofh/ofh_controller.h"
#include "ocudu/ru/ru_controller.h"

namespace ocudu {

/// \brief RU controller implementation for the Open Fronthaul interface.
///
/// Manages the timing controller that is common to all sectors and the individual controller of each sector.
class ru_sector_ofh_controller_impl : public ru_controller, public ru_operation_controller
{
public:
  explicit ru_sector_ofh_controller_impl(ocudulog::basic_logger& logger_);

  // See the ru_operation_controller interface for documentation.
  void start() override;

  // See the ru_operation_controller interface for documentation.
  void stop() override;

  // See the ru_controller interface for documentation.
  ru_operation_controller& get_operation_controller() override { return *this; }

  // See the ru_controller interface for documentation.
  ru_gain_controller* get_gain_controller() override { return nullptr; }

  // See the ru_controller interface for documentation.
  ru_cfo_controller* get_cfo_controller() override { return nullptr; }

  // See the ru_controller interface for documentation.
  ru_center_frequency_controller* get_center_frequency_controller() override { return nullptr; }

  // See the ru_controller interface for documentation.
  ru_tx_time_offset_controller* get_tx_time_offset_controller() override { return nullptr; }

  /// Sets the controller.
  void set_sector_controller(ofh::operation_controller& controller) { ofh_operation_controller = &controller; }

private:
  ocudulog::basic_logger&    logger;
  ofh::operation_controller* ofh_operation_controller;
};

} // namespace ocudu
