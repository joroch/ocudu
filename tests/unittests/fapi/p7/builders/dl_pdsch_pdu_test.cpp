/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/builders/dl_pdsch_pdu_builder.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace fapi;

TEST(dl_pdsch_pdu_builder, valid_codeword_parameters_passes)
{
  dl_pdsch_pdu         pdu;
  dl_pdsch_pdu_builder builder(pdu);

  auto builder_cw = builder.add_codeword();

  uint16_t          target_code_rate = 6;
  modulation_scheme qam_mod          = modulation_scheme::QAM64;
  sch_mcs_index     mcs_index(2);
  pdsch_mcs_table   mcs_table = pdsch_mcs_table::qam64;
  uint8_t           rv_index  = 2;
  units::bytes      tb_size{128};

  builder_cw.set_codeword_parameters(target_code_rate, qam_mod, mcs_index, mcs_table, rv_index, tb_size);

  ASSERT_EQ(mcs_table, pdu.cws[0].mcs_table);
  ASSERT_EQ(tb_size, pdu.cws[0].tb_size);
  ASSERT_EQ(mcs_index, pdu.cws[0].mcs_index);
  ASSERT_EQ(rv_index, pdu.cws[0].rv_index);
  ASSERT_EQ(qam_mod, pdu.cws[0].qam_mod_order);
  ASSERT_EQ(target_code_rate * 10, pdu.cws[0].target_code_rate);
}

TEST(dl_pdsch_pdu_builder, valid_ue_specific_parameters_passes)
{
  dl_pdsch_pdu         pdu;
  dl_pdsch_pdu_builder builder(pdu);

  rnti_t rnti = to_rnti(29);
  builder.set_ue_specific_parameters(rnti);

  ASSERT_EQ(rnti, pdu.rnti);
}

TEST(dl_pdsch_pdu_builder, valid_bwp_parameters_passes)
{
  dl_pdsch_pdu         pdu;
  dl_pdsch_pdu_builder builder(pdu);

  unsigned           bwp_size  = 128;
  unsigned           bwp_start = 192;
  cyclic_prefix      cprefix   = cyclic_prefix::NORMAL;
  subcarrier_spacing scs       = subcarrier_spacing::kHz15;

  builder.set_bwp_parameters(bwp_size, bwp_start, scs, cprefix);

  ASSERT_EQ(bwp_size, pdu.bwp_size);
  ASSERT_EQ(bwp_start, pdu.bwp_start);
  ASSERT_EQ(scs, pdu.scs);
  ASSERT_EQ(cprefix, pdu.cp);
}

TEST(dl_pdsch_pdu_builder, valid_codewords_parameters_passes)
{
  dl_pdsch_pdu         pdu;
  dl_pdsch_pdu_builder builder(pdu);

  unsigned             num_layers          = 1;
  unsigned             transmission_scheme = 0;
  pdsch_ref_point_type ref_point           = pdsch_ref_point_type::point_a;
  unsigned             n_id_pdsch          = 469;
  builder.set_codeword_information_parameters(n_id_pdsch, num_layers, transmission_scheme, ref_point);

  ASSERT_EQ(n_id_pdsch, pdu.nid_pdsch);
  ASSERT_EQ(num_layers, pdu.num_layers);
  ASSERT_EQ(transmission_scheme, pdu.transmission_scheme);
  ASSERT_EQ(ref_point, pdu.ref_point);
}

TEST(dl_pdsch_pdu_builder, add_codeword_passes)
{
  dl_pdsch_pdu         pdu;
  dl_pdsch_pdu_builder builder(pdu);
  ASSERT_TRUE(pdu.cws.empty());

  builder.add_codeword();
  ASSERT_EQ(1, pdu.cws.size());
}

TEST(dl_pdsch_pdu_builder, valid_dmrs_parameters_passes)
{
  dl_pdsch_pdu         pdu;
  dl_pdsch_pdu_builder builder(pdu);

  unsigned         dmrs_scrambling_id            = 20;
  unsigned         dmrs_scrambling_id_complement = 30;
  dmrs_symbol_mask dmrs_symbol_pos(13);
  dmrs_symbol_pos.from_uint64(3);
  dmrs_ports_mask dmrs_ports(11);
  dmrs_ports.from_uint64(4);
  unsigned         num_dmrs_cdm_grp_no_data = 2;
  unsigned         nscid                    = 1;
  dmrs_config_type config_type              = dmrs_config_type::type1;

  builder.set_dmrs_parameters(dmrs_symbol_pos,
                              config_type,
                              dmrs_scrambling_id,
                              dmrs_scrambling_id_complement,
                              nscid,
                              num_dmrs_cdm_grp_no_data,
                              dmrs_ports);

  ASSERT_EQ(dmrs_symbol_pos, pdu.dl_dmrs_symb_pos);
  ASSERT_EQ(config_type, pdu.dmrs_type);
  ASSERT_EQ(dmrs_scrambling_id, pdu.pdsch_dmrs_scrambling_id);
  ASSERT_EQ(dmrs_scrambling_id_complement, pdu.pdsch_dmrs_scrambling_id_compl);
  ASSERT_EQ(nscid, pdu.nscid);
  ASSERT_EQ(num_dmrs_cdm_grp_no_data, pdu.num_dmrs_cdm_grps_no_data);
  ASSERT_EQ(dmrs_ports, pdu.dmrs_ports);
}

TEST(dl_pdsch_pdu_builder, valid_freq_allocation_type_0_parameters_passes)
{
  dl_pdsch_pdu         pdu;
  dl_pdsch_pdu_builder builder(pdu);

  std::vector<uint8_t>     bitmap(3, 1);
  vrb_to_prb::mapping_type vrb_to_prb = vrb_to_prb::mapping_type::non_interleaved;
  builder.set_pdsch_allocation_in_frequency_type_0(span<const uint8_t>(bitmap), vrb_to_prb);

  ASSERT_EQ(vrb_to_prb, pdu.vrb_to_prb_mapping);
}

TEST(dl_pdsch_pdu_builder, valid_freq_allocation_type_1_parameters_passes)
{
  dl_pdsch_pdu         pdu;
  dl_pdsch_pdu_builder builder(pdu);

  vrb_to_prb::mapping_type vrb_to_prb = vrb_to_prb::mapping_type::interleaved_n2;
  uint16_t                 rb_start   = 15;
  uint16_t                 rb_size    = 20;

  builder.set_pdsch_allocation_in_frequency_type_1(rb_start, rb_size, vrb_to_prb);

  ASSERT_EQ(vrb_to_prb, pdu.vrb_to_prb_mapping);
  const auto* ra_type_1 = std::get_if<resource_allocation_type_1>(&pdu.resource_alloc);
  ASSERT_TRUE(ra_type_1);
  ASSERT_EQ(rb_start, ra_type_1->rb_start);
  ASSERT_EQ(rb_size, ra_type_1->rb_size);
}

TEST(dl_pdsch_pdu_builder, valid_time_allocation_parameters_passes)
{
  dl_pdsch_pdu         pdu;
  dl_pdsch_pdu_builder builder(pdu);

  uint8_t start_symb = 4;
  uint8_t nof_symb   = 8;

  builder.set_pdsch_allocation_in_time_parameters(start_symb, nof_symb);

  ASSERT_EQ(start_symb, pdu.start_symbol_index);
  ASSERT_EQ(nof_symb, pdu.nr_of_symbols);
}

TEST(dl_pdsch_pdu_builder, valid_tx_power_info_parameters_passes)
{
  for (int power : {-8, 16}) {
    dl_pdsch_pdu         pdu;
    dl_pdsch_pdu_builder builder(pdu);

    power_control_offset_ss ss_profile = power_control_offset_ss::dB0;

    builder.set_profile_nr_tx_power_info_parameters(power, ss_profile);

    const auto* profile_nr = std::get_if<dl_pdsch_pdu::power_profile_nr>(&pdu.power_config);
    ASSERT_TRUE(profile_nr != nullptr);
    ASSERT_EQ(power, profile_nr->power_control_offset_profile_nr);
    ASSERT_EQ(ss_profile, profile_nr->power_control_offset_ss_profile_nr);
  }
}

TEST(dl_pdsch_pdu_builder, valid_tx_power_profile_sss_info_parameters_passes)
{
  dl_pdsch_pdu         pdu;
  dl_pdsch_pdu_builder builder(pdu);

  float dmrs = 22.5;
  float data = -10.99;

  builder.set_profile_sss_tx_power_info_parameters(dmrs, data);

  const auto* profile_sss = std::get_if<dl_pdsch_pdu::power_profile_sss>(&pdu.power_config);
  ASSERT_TRUE(profile_sss != nullptr);
  ASSERT_FLOAT_EQ(dmrs, profile_sss->dmrs_power_offset_sss_db);
  ASSERT_FLOAT_EQ(data, profile_sss->data_power_offset_sss_db);
}

TEST(dl_pdsch_pdu_builder, valid_maintenance_v3_parameters_passes)
{
  dl_pdsch_pdu         pdu;
  dl_pdsch_pdu_builder builder(pdu);

  pdsch_trans_type trans_type = pdsch_trans_type::interleaved_common_any_coreset0_not_present;

  builder.set_maintenance_v3_bwp_parameters(trans_type);

  ASSERT_EQ(trans_type, pdu.pdsch_maintenance_v3.trans_type);
}

TEST(dl_pdsch_pdu_builder, valid_set_ldpc_base_graph_passes)
{
  for (auto graph_type : {ldpc_base_graph_type::BG2, ldpc_base_graph_type::BG1}) {
    dl_pdsch_pdu         pdu;
    dl_pdsch_pdu_builder builder(pdu);

    units::bytes tb_size_lbrm_bytes{15};
    builder.set_ldpc_base_graph(graph_type);

    ASSERT_EQ(graph_type, pdu.ldpc_base_graph);
  }
}

TEST(dl_pdsch_pdu_builder, valid_maintenance_v3_csi_rm_parameters_passes)
{
  dl_pdsch_pdu         pdu;
  dl_pdsch_pdu_builder builder(pdu);

  static_vector<uint16_t, 16> csi = {0, 1, 2};

  builder.set_csi_rm_references({csi});

  ASSERT_EQ(csi, pdu.csi_for_rm);
}
