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

#include "ocudu/fapi/p7/messages/dmrs_definitions.h"
#include "ocudu/fapi/p7/messages/power_control_offset_ss.h"
#include "ocudu/fapi/p7/messages/pxsch_parameters.h"
#include "ocudu/fapi/p7/messages/resource_allocation_types.h"
#include "ocudu/fapi/p7/messages/tx_precoding_and_beamforming_pdu.h"
#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/dmrs/dmrs.h"
#include "ocudu/ran/pdsch/pdsch_context.h"
#include "ocudu/ran/resource_allocation/vrb_to_prb.h"
#include "ocudu/ran/sch/ldpc_base_graph.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include "ocudu/scheduler/result/pdsch_info.h"
#include <variant>

namespace ocudu {
namespace fapi {

enum class pdsch_trans_type : uint8_t {
  non_interleaved_common_ss,
  non_interleaved_other,
  interleaved_common_type0_coreset0,
  interleaved_common_any_coreset0_present,
  interleaved_common_any_coreset0_not_present,
  interleaved_other
};

/// \note For this release num_coreset_rm_patterns = 0.
/// \note For this release num_prb_sym_rm_patts_by_value = 0.
struct dl_pdsch_maintenance_parameters_v3 {
  pdsch_trans_type trans_type; // TODO: Tocara borrarlo, pero hay que pensar...
};

/// Codeword information.
struct dl_pdsch_codeword {
  uint16_t          target_code_rate;
  modulation_scheme qam_mod_order;
  sch_mcs_index     mcs_index;
  pdsch_mcs_table   mcs_table;
  uint8_t           rv_index;
  units::bytes      tb_size;
};

enum class inline_tb_crc_type : uint8_t { data_payload, control_message };
enum class pdsch_ref_point_type : uint8_t { point_a, subcarrier_0 };

/// Downlink PDSCH PDU information.
struct dl_pdsch_pdu {
  /// Bit position of the first TB in the is_last_cb_present bitmap.
  static constexpr unsigned LAST_CB_BITMAP_FIRST_TB_BIT = 0U;
  /// Bit position of the second TB in the is_last_cb_present bitmap.
  static constexpr unsigned LAST_CB_BITMAP_SECOND_TB_BIT = 1U;

  /// Maximum number of codewords per PDU.
  static constexpr unsigned MAX_NUM_CW_PER_PDU = 2;
  /// Maximum size of DL TB CRC.
  static constexpr unsigned MAX_SIZE_DL_TB_CRC = 2;

  /// Profile NR power parameters.
  struct power_profile_nr {
    int                     power_control_offset_profile_nr;
    power_control_offset_ss power_control_offset_ss_profile_nr;
  };

  /// Profile SSS power parameters.
  struct power_profile_sss {
    float dmrs_power_offset_sss_db;
    float data_power_offset_sss_db;
  };

  rnti_t                                               rnti;
  uint16_t                                             pdu_index; // TODO: Can we delete this?
  uint16_t                                             bwp_size;
  uint16_t                                             bwp_start;
  subcarrier_spacing                                   scs;
  cyclic_prefix                                        cp;
  static_vector<dl_pdsch_codeword, MAX_NUM_CW_PER_PDU> cws;
  uint16_t                                             nid_pdsch;
  uint8_t                                              num_layers;
  uint8_t                                              transmission_scheme;
  pdsch_ref_point_type                                 ref_point;
  dmrs_symbol_mask                                     dl_dmrs_symb_pos;
  uint16_t                                             pdsch_dmrs_scrambling_id;
  dmrs_config_type                                     dmrs_type;
  uint16_t                                             pdsch_dmrs_scrambling_id_compl;
  uint8_t                                              nscid;
  uint8_t                                              num_dmrs_cdm_grps_no_data;
  dmrs_ports_mask                                      dmrs_ports;
  resource_allocation_types                            resource_alloc;
  vrb_to_prb::mapping_type                             vrb_to_prb_mapping;
  uint8_t                                              start_symbol_index;
  uint8_t                                              nr_of_symbols;
  std::variant<power_profile_nr, power_profile_sss>    power_config;
  tx_precoding_and_beamforming_pdu                     precoding_and_beamforming;
  dl_pdsch_maintenance_parameters_v3                   pdsch_maintenance_v3;
  ldpc_base_graph_type                                 ldpc_base_graph;
  static_vector<uint16_t, 16>                          csi_for_rm; // TODO: This should not be a vector
  /// Vendor specific parameters.
  std::optional<pdsch_context> context;
};

} // namespace fapi
} // namespace ocudu
