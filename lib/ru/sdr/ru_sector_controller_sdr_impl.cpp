/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_sector_controller_sdr_impl.h"
#include "lower_phy/lower_phy_sector.h"
#include "ocudu/gateways/baseband/buffer/baseband_gateway_buffer_reader.h"
#include "ocudu/phy/lower/lower_phy_controller.h"
#include "ocudu/phy/lower/processors/lower_phy_center_freq_controller.h"
#include "ocudu/phy/lower/processors/lower_phy_cfo_controller.h"
#include "ocudu/phy/lower/processors/lower_phy_tx_time_offset_controller.h"
#include "ocudu/radio/radio_session.h"
#include <thread>

using namespace ocudu;

void ru_sector_controller_sdr_impl::start()
{
  ocudu_assert(low_phy, "Null lower PHY");

  // Start streaming at the given time.
  if (sfn0_ref_time.has_value()) {
    low_phy->get_controller().start(start_time, *sfn0_ref_time);

    return;
  }

  low_phy->get_controller().start(start_time);
}

void ru_sector_controller_sdr_impl::stop()
{
  low_phy->get_controller().stop();
}

void ru_sector_controller_sdr_impl::set_lower_phy_sector(lower_phy_sector& sector)
{
  low_phy = &sector;

  gain_controller           = ru_gain_controller_sdr_impl(&radio);
  cfo_controller            = ru_cfo_controller_sdr_impl(sector);
  center_freq_controller    = ru_center_frequency_controller_sdr_impl(sector, &radio);
  tx_time_offset_controller = ru_tx_time_offset_controller_sdr_impl(sector);
}

bool ru_gain_controller_sdr_impl::set_tx_gain(unsigned port_id, double gain_dB)
{
  return (*radio)->get_management_plane().set_tx_gain(port_id, gain_dB);
}

bool ru_gain_controller_sdr_impl::set_rx_gain(unsigned port_id, double gain_dB)
{
  return (*radio)->get_management_plane().set_rx_gain(port_id, gain_dB);
}

bool ru_cfo_controller_sdr_impl::set_tx_cfo(unsigned sector_id, const cfo_compensation_request& cfo_request)
{
  if (!low_phy) {
    return false;
  }
  return low_phy->get_tx_cfo_control().schedule_cfo_command(
      cfo_request.start_timestamp.value_or(std::chrono::system_clock::now()),
      cfo_request.cfo_hz,
      cfo_request.cfo_drift_hz_s);
}

bool ru_cfo_controller_sdr_impl::set_rx_cfo(unsigned sector_id, const cfo_compensation_request& cfo_request)
{
  if (!low_phy) {
    return false;
  }

  return low_phy->get_rx_cfo_control().schedule_cfo_command(
      cfo_request.start_timestamp.value_or(std::chrono::system_clock::now()),
      cfo_request.cfo_hz,
      cfo_request.cfo_drift_hz_s);
}

bool ru_center_frequency_controller_sdr_impl::set_tx_center_frequency(unsigned sector_id, double center_freq_Hz)
{
  if (!low_phy || !radio) {
    return false;
  }

  (*radio)->get_management_plane().set_tx_freq(sector_id, center_freq_Hz);
  return low_phy->get_tx_center_freq_control().set_carrier_center_frequency(center_freq_Hz);
}

bool ru_center_frequency_controller_sdr_impl::set_rx_center_frequency(unsigned sector_id, double center_freq_Hz)
{
  if (!low_phy || !radio) {
    return false;
  }

  (*radio)->get_management_plane().set_rx_freq(sector_id, center_freq_Hz);
  return low_phy->get_rx_center_freq_control().set_carrier_center_frequency(center_freq_Hz);
}

bool ru_tx_time_offset_controller_sdr_impl::set_tx_time_offset(unsigned sector_id, phy_time_unit tx_time_offset)
{
  if (!low_phy) {
    return false;
  }

  low_phy->get_tx_time_offset_control().set_tx_time_offset(tx_time_offset);
  return true;
}
