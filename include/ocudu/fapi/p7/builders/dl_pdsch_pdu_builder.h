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

#include "ocudu/fapi/p7/builders/message_builder_helper.h"
#include "ocudu/fapi/p7/builders/tx_precoding_and_beamforming_pdu_builder.h"
#include "ocudu/fapi/p7/messages/dl_pdsch_pdu.h"
#include "ocudu/fapi/p7/messages/power_control_offset_ss.h"
#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/dmrs/dmrs.h"
#include "ocudu/ran/subcarrier_spacing.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// Builder that helps to fill the parameters of a DL PDSCH codeword.
class dl_pdsch_codeword_builder
{
public:
  dl_pdsch_codeword_builder(dl_pdsch_codeword& cw_) : cw(cw_) {}

  /// Sets the codeword basic parameters.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_codeword_builder& set_codeword_parameters(float             target_code_rate,
                                                     modulation_scheme qam_mod,
                                                     sch_mcs_index     mcs_index,
                                                     pdsch_mcs_table   mcs_table,
                                                     uint8_t           rv_index,
                                                     units::bytes      tb_size)
  {
    cw.target_code_rate = target_code_rate * 10.F;
    cw.qam_mod_order    = qam_mod;
    cw.mcs_index        = mcs_index;
    cw.mcs_table        = mcs_table;
    cw.rv_index         = rv_index;
    cw.tb_size          = tb_size;

    return *this;
  }

private:
  dl_pdsch_codeword& cw;
};

/// DL PDSCH PDU builder that helps to fill the parameters specified in SCF-222 v4.0 section 3.4.2.2.
class dl_pdsch_pdu_builder
{
public:
  explicit dl_pdsch_pdu_builder(dl_pdsch_pdu& pdu_) : pdu(pdu_) {}

  /// Returns the PDU index.
  unsigned get_pdu_id() const { return pdu.pdu_index; }

  /// Sets the UE specific parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_ue_specific_parameters(rnti_t rnti)
  {
    pdu.rnti = rnti;

    return *this;
  }

  /// Sets the BWP parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder&
  set_bwp_parameters(uint16_t bwp_size, uint16_t bwp_start, subcarrier_spacing scs, cyclic_prefix cp)
  {
    pdu.bwp_size  = bwp_size;
    pdu.bwp_start = bwp_start;
    pdu.scs       = scs;
    pdu.cp        = cp;

    return *this;
  }

  /// Adds a codeword to the PDSCH PDU and returns a codeword builder to fill the codeword parameters.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_codeword_builder add_codeword()
  {
    dl_pdsch_codeword_builder builder(pdu.cws.emplace_back());

    return builder;
  }

  /// Sets the codeword information parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_codeword_information_parameters(uint16_t             n_id_pdsch,
                                                            uint8_t              num_layers,
                                                            uint8_t              trasnmission_scheme,
                                                            pdsch_ref_point_type ref_point)
  {
    pdu.nid_pdsch           = n_id_pdsch;
    pdu.num_layers          = num_layers;
    pdu.transmission_scheme = trasnmission_scheme;
    pdu.ref_point           = ref_point;

    return *this;
  }

  /// Sets the DMRS parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_dmrs_parameters(dmrs_symbol_mask dl_dmrs_symb_pos,
                                            dmrs_config_type dmrs_type,
                                            uint16_t         pdsch_dmrs_scrambling_id,
                                            uint16_t         pdsch_dmrs_scrambling_id_complement,
                                            uint8_t          nscid,
                                            uint8_t          num_dmrs_cdm_groups_no_data,
                                            dmrs_ports_mask  dmrs_ports)
  {
    pdu.dl_dmrs_symb_pos               = dl_dmrs_symb_pos;
    pdu.dmrs_type                      = dmrs_type;
    pdu.pdsch_dmrs_scrambling_id       = pdsch_dmrs_scrambling_id;
    pdu.pdsch_dmrs_scrambling_id_compl = pdsch_dmrs_scrambling_id_complement;
    pdu.nscid                          = nscid;
    pdu.num_dmrs_cdm_grps_no_data      = num_dmrs_cdm_groups_no_data;
    pdu.dmrs_ports                     = dmrs_ports;

    return *this;
  }

  /// Sets the PDSCH allocation in frequency type 0 parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_pdsch_allocation_in_frequency_type_0(span<const uint8_t>      rb_map,
                                                                 vrb_to_prb::mapping_type vrb_to_prb_mapping)
  {
    auto& ra_type0         = pdu.resource_alloc.emplace<resource_allocation_type_0>();
    pdu.vrb_to_prb_mapping = vrb_to_prb_mapping;

    ocudu_assert(rb_map.size() <= RB_BITMAP_SIZE_IN_BYTES,
                 "[PDSCH Builder] - Incoming RB bitmap size {} exceeds FAPI bitmap field {}",
                 rb_map.size(),
                 int(RB_BITMAP_SIZE_IN_BYTES));

    std::copy(rb_map.begin(), rb_map.end(), ra_type0.rb_bitmap.begin());

    return *this;
  }

  /// Sets the PDSCH allocation in frequency type 1 parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_pdsch_allocation_in_frequency_type_1(uint16_t                 rb_start,
                                                                 uint16_t                 rb_size,
                                                                 vrb_to_prb::mapping_type vrb_to_prb_mapping)
  {
    auto& ra_type1         = pdu.resource_alloc.emplace<resource_allocation_type_1>();
    ra_type1.rb_start      = rb_start;
    ra_type1.rb_size       = rb_size;
    pdu.vrb_to_prb_mapping = vrb_to_prb_mapping;

    return *this;
  }

  /// Sets the PDSCH allocation in time parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_pdsch_allocation_in_time_parameters(uint8_t start_symbol_index, uint8_t nof_symbols)
  {
    pdu.start_symbol_index = start_symbol_index;
    pdu.nr_of_symbols      = nof_symbols;

    return *this;
  }

  /// Sets the profile NR Tx Power info parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_profile_nr_tx_power_info_parameters(int                     power_control_offset,
                                                                power_control_offset_ss power_control_offset_ss)
  {
    auto& power = pdu.power_config.emplace<dl_pdsch_pdu::power_profile_nr>();

    power.power_control_offset_profile_nr    = power_control_offset;
    power.power_control_offset_ss_profile_nr = power_control_offset_ss;

    return *this;
  }

  /// Sets the profile SSS Tx Power info parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_profile_sss_tx_power_info_parameters(float dmrs_offset_db, float data_offset_db)
  {
    auto& power                    = pdu.power_config.emplace<dl_pdsch_pdu::power_profile_sss>();
    power.data_power_offset_sss_db = data_offset_db;
    power.dmrs_power_offset_sss_db = dmrs_offset_db;

    return *this;
  }

  /// Sets the maintenance v3 BWP information parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder& set_maintenance_v3_bwp_parameters(pdsch_trans_type trans_type)
  {
    pdu.pdsch_maintenance_v3.trans_type = trans_type;

    return *this;
  }

  /// Sets the LDPC base graph for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder& set_ldpc_base_graph(ldpc_base_graph_type ldpc_base_graph)
  {
    pdu.ldpc_base_graph = ldpc_base_graph;

    return *this;
  }

  /// Sets the CSI-RS rate matching references parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder& set_csi_rm_references(span<const uint16_t> csi_rs_for_rm)
  {
    pdu.csi_for_rm.assign(csi_rs_for_rm.begin(), csi_rs_for_rm.end());

    return *this;
  }

  /// Sets the PDSCH context as vendor specific.
  dl_pdsch_pdu_builder& set_context_vendor_specific(harq_id_t harq_id, unsigned k1, unsigned nof_retxs)
  {
    pdu.context = pdsch_context(harq_id, k1, nof_retxs);
    return *this;
  }

  /// Returns a transmission precoding and beamforming PDU builder of this PDSCH PDU.
  tx_precoding_and_beamforming_pdu_builder get_tx_precoding_and_beamforming_pdu_builder()
  {
    tx_precoding_and_beamforming_pdu_builder builder(pdu.precoding_and_beamforming);

    return builder;
  }

private:
  dl_pdsch_pdu& pdu;
};

} // namespace fapi
} // namespace ocudu
