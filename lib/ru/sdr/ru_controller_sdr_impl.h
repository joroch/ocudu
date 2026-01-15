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

#include "ocudu/radio/radio_session.h"
#include "ocudu/ru/ru_controller.h"
#include <vector>

namespace ocudu {
class ru_sector_controller_sdr_impl;

/// SDR Radio Unit controller implementation.
class ru_controller_sdr_impl : public ru_operation_controller
{
public:
  ru_controller_sdr_impl(double srate_MHz_, std::optional<std::chrono::system_clock::time_point> start_time);

  // See interface for documentation.
  void start() override;

  // See interface for documentation.
  void stop() override;

  /// Sets the radio session of this RU operation controller.
  void set_radio(radio_session& session) { radio = &session; }

  /// Sets the sector operation controllers.
  void set_sector_controllers(std::vector<ru_sector_controller_sdr_impl*> controllers)
  {
    sector_controllers = std::move(controllers);
  }

private:
  double                                               srate_MHz;
  std::optional<std::chrono::system_clock::time_point> start_time;
  radio_session*                                       radio = nullptr;
  std::vector<ru_sector_controller_sdr_impl*>          sector_controllers;
};

} // namespace ocudu
