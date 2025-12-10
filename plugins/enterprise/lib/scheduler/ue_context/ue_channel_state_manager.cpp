/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ue_context/ue_channel_state_manager.h"

using namespace ocudu;

unsigned ue_channel_state_manager::get_nof_ul_layers() const
{
  // SINR maximum differential between the highest and lowest SINR in decibels. The number of layers is ignored if the
  // differential is greater than the threshold.
  static constexpr float sinr_dB_max_diff = 6.0F;
  // SINR threshold in decibels: to select a number of layers, the SINR of all layers must be greater than the
  // threshold.
  static constexpr float sinr_dB_threshold = 18.0F;
  // SINR comparison penalty in decibels. A lower number of layers is selected only if its SINR is larger than the SINR
  // of a higher number of layers plus the penalty.
  static constexpr float sinr_dB_penalty = 2.0F;

  // Skip if it does not have a TPMI.
  if (!last_pusch_tpmi_select_info.has_value()) {
    return 1;
  }

  unsigned max_nof_layers = last_pusch_tpmi_select_info->get_max_nof_layers();

  // Find maximum number of layers.
  float    best_sinr       = -std::numeric_limits<float>::infinity();
  unsigned best_nof_layers = 1;
  for (unsigned nof_layers = max_nof_layers; nof_layers != 0; --nof_layers) {
    // Obtain TPMI information for the number of layers.
    const pusch_tpmi_select_info::tpmi_info& info = last_pusch_tpmi_select_info->get_tpmi_select(nof_layers);

    // Get the minimum SINR.
    float min_sinr_dB = *std::min_element(info.sinr_dB_layer.begin(), info.sinr_dB_layer.end());

    // Get the maximum SINR.
    float max_sinr_dB = *std::max_element(info.sinr_dB_layer.begin(), info.sinr_dB_layer.end());

    // If the minimum SINR is above the threshold, use this number of layers.
    if (min_sinr_dB > sinr_dB_threshold) {
      return nof_layers;
    }

    // Discard number of layers if the differential between the maximum and minimum exceeds the limit.
    if (max_sinr_dB - min_sinr_dB > sinr_dB_max_diff) {
      continue;
    }

    // If the minimum SINR is better than the previous best above the hysteresis, select this number of layers.
    if (info.avg_sinr_dB > best_sinr) {
      best_sinr       = info.avg_sinr_dB + sinr_dB_penalty;
      best_nof_layers = nof_layers;
    }
  }

  return best_nof_layers;
}
