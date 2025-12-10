/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ue_capability_manager.h"
#include "ocudu/asn1/rrc_nr/ul_dcch_msg_ies.h"

using namespace ocudu;
using namespace odu;

void ocudu::odu::decode_advanced_ue_nr_caps(odu::ue_capability_summary&      ue_capability,
                                            const asn1::rrc_nr::ue_nr_cap_s& ue_cap)
{
  for (const auto& band : ue_cap.rf_params.supported_band_list_nr) {
    // Select band.
    nr_band                                band_id  = static_cast<nr_band>(band.band_nr);
    ue_capability_summary::supported_band& band_cap = ue_capability.bands.at(band_id);

    // Convert UL-MIMO related parameter.
    tx_scheme_codebook_subset pusch_tx_coherence = tx_scheme_codebook_subset::non_coherent;
    if (band.mimo_params_per_band_present) {
      if (band.mimo_params_per_band.pusch_trans_coherence_present) {
        switch (band.mimo_params_per_band.pusch_trans_coherence) {
          case asn1::rrc_nr::mimo_params_per_band_s::pusch_trans_coherence_opts::non_coherent:
          case asn1::rrc_nr::mimo_params_per_band_s::pusch_trans_coherence_opts::nulltype:
            pusch_tx_coherence = tx_scheme_codebook_subset::non_coherent;
            break;
          case asn1::rrc_nr::mimo_params_per_band_s::pusch_trans_coherence_opts::partial_coherent:
            pusch_tx_coherence = tx_scheme_codebook_subset::partial_and_non_coherent;
            break;
          case asn1::rrc_nr::mimo_params_per_band_s::pusch_trans_coherence_opts::full_coherent:
            pusch_tx_coherence = tx_scheme_codebook_subset::fully_and_partial_and_non_coherent;
            break;
        }
      }
    }
    band_cap.pusch_tx_coherence = pusch_tx_coherence;
  }

  for (const auto& band_combination : ue_cap.rf_params.supported_band_combination_list) {
    // Ignore empty and CA band combinations.
    if (band_combination.band_list.size() != 1) {
      continue;
    }

    // Skip if the band parameters is not for NR.
    const asn1::rrc_nr::band_params_c& band_params = band_combination.band_list[0];
    if (band_params.type() != asn1::rrc_nr::band_params_c::types_opts::nr) {
      continue;
    }

    // Get band identifier. Skip if the band is not in the band list.
    nr_band band_id = static_cast<nr_band>(band_params.nr().band_nr);
    if (ue_capability.bands.count(band_id) == 0) {
      continue;
    }
    ue_capability_summary::supported_band& band_cap = ue_capability.bands.at(band_id);

    // Select feature set identifier.
    uint16_t feature_set_id = band_combination.feature_set_combination;

    // Skip if the selected feature set combination is empty or not for NR.
    if ((ue_cap.feature_set_combinations.size() <= feature_set_id) ||
        (ue_cap.feature_set_combinations[feature_set_id].size() == 0) ||
        (ue_cap.feature_set_combinations[feature_set_id][0].size() == 0) ||
        (ue_cap.feature_set_combinations[feature_set_id][0][0].type() != asn1::rrc_nr::feature_set_c::types_opts::nr)) {
      continue;
    }

    // Extract feature set identifiers for each direction.
    const auto& feature_set   = ue_cap.feature_set_combinations[feature_set_id][0][0].nr();
    uint16_t    feature_ul_id = feature_set.ul_set_nr;
    uint16_t    feature_dl_id = feature_set.dl_set_nr;

    // Skip if any of the feature identifiers is zero.
    if ((feature_ul_id == 0) || (feature_dl_id == 0)) {
      continue;
    }
    --feature_ul_id;
    --feature_dl_id;

    // Skip if the selected UL feature set combination is empty or not for NR.
    if (ue_cap.feature_sets.feature_sets_ul.size() <= feature_ul_id) {
      continue;
    }

    // Select feature set for UL.
    const auto& feature_set_ul = ue_cap.feature_sets.feature_sets_ul[feature_ul_id];

    // Parse SRS capabilities.
    if (feature_set_ul.supported_srs_res_features_present) {
      switch (feature_set_ul.supported_srs_res_features.max_num_srs_ports_per_res) {
        case asn1::rrc_nr::srs_res_features_s::max_num_srs_ports_per_res_opts::n1:
        case asn1::rrc_nr::srs_res_features_s::max_num_srs_ports_per_res_opts::nulltype:
          band_cap.nof_srs_tx_ports = std::max(band_cap.nof_srs_tx_ports, static_cast<uint8_t>(1U));
          break;
        case asn1::rrc_nr::srs_res_features_s::max_num_srs_ports_per_res_opts::n2:
          band_cap.nof_srs_tx_ports = std::max(band_cap.nof_srs_tx_ports, static_cast<uint8_t>(2U));
          break;
        case asn1::rrc_nr::srs_res_features_s::max_num_srs_ports_per_res_opts::n4:
          band_cap.nof_srs_tx_ports = std::max(band_cap.nof_srs_tx_ports, static_cast<uint8_t>(4U));
          break;
      }
    }

    // Select the UL feature per CC identifier.
    uint16_t feature_ul_per_cc_id = feature_set_ul.feature_set_list_per_ul_cc[0];

    // Skip if the identifier is zero.
    if (feature_ul_per_cc_id == 0) {
      continue;
    }
    --feature_ul_per_cc_id;

    // Extract UL feature set per CC.
    const auto& feature_set_per_cc_ul = ue_cap.feature_sets.feature_sets_ul_per_cc[feature_ul_per_cc_id];

    // Parse maximum number of layers.
    if (feature_set_per_cc_ul.mimo_cb_pusch_present &&
        feature_set_per_cc_ul.mimo_cb_pusch.max_num_mimo_layers_cb_pusch_present) {
      switch (feature_set_per_cc_ul.mimo_cb_pusch.max_num_mimo_layers_cb_pusch) {
        case asn1::rrc_nr::mimo_layers_ul_opts::one_layer:
        case asn1::rrc_nr::mimo_layers_ul_opts::nulltype:
          band_cap.pusch_max_rank = std::max(band_cap.pusch_max_rank, 1U);
          break;
        case asn1::rrc_nr::mimo_layers_ul_opts::two_layers:
          band_cap.pusch_max_rank = std::max(band_cap.pusch_max_rank, 2U);
          break;
        case asn1::rrc_nr::mimo_layers_ul_opts::four_layers:
          band_cap.pusch_max_rank = std::max(band_cap.pusch_max_rank, 4U);
          break;
      }
    }
  }
}
