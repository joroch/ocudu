/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_controller_sdr_impl.h"
#include "ru_sector_controller_sdr_impl.h"
#include "ocudu/gateways/baseband/buffer/baseband_gateway_buffer_reader.h"
#include "ocudu/radio/radio_session.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/support/math/math_utils.h"
#include <thread>

using namespace ocudu;

ru_controller_sdr_impl::ru_controller_sdr_impl(double                                               srate_MHz_,
                                               std::optional<std::chrono::system_clock::time_point> start_time_) :
  srate_MHz(srate_MHz_), start_time(start_time_)
{
}

void ru_controller_sdr_impl::start()
{
  ocudu_assert(radio, "Null radio");
  ocudu_assert(!sector_controllers.empty(), "Empty list of lower phy sectors");

  // Start streaming at the given time.
  if (start_time.has_value()) {
    // Sleep until the start time.
    std::this_thread::sleep_until(*start_time);

    // Get current radio timestamp.
    baseband_gateway_timestamp current_radio_ts = radio->read_current_time();

    // Calculate some constants that depend on the sampling rate.
    uint64_t nof_ticks_per_second  = static_cast<uint64_t>(srate_MHz * 1e6);
    uint64_t nof_ticks_subframe    = nof_ticks_per_second / 1000;
    uint64_t nof_ticks_super_frame = NOF_SUBFRAMES_PER_FRAME * NOF_SFNS * nof_ticks_subframe;
    uint64_t start_advance_10ms    = nof_ticks_per_second / 100;

    // Round time to the next 1PPS rising-edge.
    baseband_gateway_timestamp time_next_pps =
        divide_ceil(current_radio_ts, nof_ticks_per_second) * nof_ticks_per_second;

    // Calculate the SFN0 reference time for matching the SFN0 to the 1PPS rising edge.
    baseband_gateway_timestamp sfn0_ref_time = time_next_pps % nof_ticks_super_frame;

    // Start streaming 10-ms before the 1PPS rising edge.
    baseband_gateway_timestamp time_start = time_next_pps - start_advance_10ms;

    // Update the sector controllers with the start time and SF0 reference time.
    for (auto& sector : sector_controllers) {
      sector->update_start_time(time_start, sfn0_ref_time);
    }

    // Start radio and lower physical layer at the given timestamp.
    radio->start(time_start);
    for (auto& sector : sector_controllers) {
      sector->get_operation_controller().start();
    }

    return;
  }

  // Calculate starting time from the radio current time plus one hundred milliseconds.
  double                     delay_s      = 0.1;
  baseband_gateway_timestamp current_time = radio->read_current_time();
  baseband_gateway_timestamp start_ts     = current_time + static_cast<uint64_t>(delay_s * srate_MHz * 1e6);

  // Round start time to the next subframe.
  uint64_t sf_duration = static_cast<uint64_t>(srate_MHz * 1e3);
  start_ts             = divide_ceil(start_ts, sf_duration) * sf_duration;

  // Update the sector controllers with the start time.
  for (auto& sector : sector_controllers) {
    sector->update_start_time(start_ts, {});
  }

  // Start radio and lower physical layer at the given timestamp.
  radio->start(start_ts);
  for (auto& sector : sector_controllers) {
    sector->get_operation_controller().start();
  }
}

void ru_controller_sdr_impl::stop()
{
  radio->stop();

  for (auto& sector : sector_controllers) {
    sector->stop();
  }
}
