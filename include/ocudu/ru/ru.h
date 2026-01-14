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

namespace ocudu {

// :TODO: temporal forward to start the RU
class ru_operation_controller;

class ru_controller;
class ru_downlink_plane_handler;
class ru_metrics_collector;
class ru_uplink_plane_handler;

/// \brief Radio Unit sector interface.
///
/// The Radio Unit sector interface provides downlink and uplink functionality through the uplink and downlink planes,
/// allowing data transmission and reception using a radio. It also notifies timing events using the \ref
/// ru_timing_notifier.
class radio_unit_sector
{
public:
  /// Default destructor.
  virtual ~radio_unit_sector() = default;

  /// Returns the controller interface of this Radio Unit sector.
  virtual ru_controller& get_controller() = 0;

  /// Returns the downlink plane handler interface of this Radio Unit sector.
  virtual ru_downlink_plane_handler& get_downlink_plane_handler() = 0;

  /// Returns the uplink plane interface handler of this Radio Unit sector.
  virtual ru_uplink_plane_handler& get_uplink_plane_handler() = 0;
};

/// \brief Radio Unit interface.
///
/// The Radio Unit interface provides downlink and uplink functionality through the uplink and downlink planes, allowing
/// data transmission and reception using a radio. Only a single Radio Unit interface should be used when operating with
/// multiple sectors.
class radio_unit
{
public:
  /// Default destructor.
  virtual ~radio_unit() = default;

  /// Returns the operation controller of this Radio Unit.
  // :TODO: temporal method to start the RU
  virtual ru_operation_controller& get_operation_controller() = 0;

  /// Returns the sector managed by this Radio Unit using the given sector identifier or nullptr if the sector is not
  /// managed by this Radio Unit.
  virtual radio_unit_sector* get_radio_unit_sector(unsigned sector_id) = 0;

  /// Returns the metrics collector of this Radio Unit.
  virtual ru_metrics_collector* get_metrics_collector() = 0;
};

} // namespace ocudu
