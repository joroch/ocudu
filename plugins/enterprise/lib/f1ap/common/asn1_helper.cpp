/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "asn1_helper.h"
#include "ocudu/asn1/f1ap/f1ap_ies.h"
#include "ocudu/ran/resource_block.h"
#include "ocudu/ran/srs/srs_configuration.h"

using namespace ocudu;
using namespace odu;
using namespace asn1::f1ap;

asn1::f1ap::trp_info_s asn1_helper::trp_info_to_asn1(const odu::du_trp_info& trp)
{
  trp_info_s asn1out;

  asn1out.trp_id = trp_id_to_uint(trp.trp_id);
  if (trp.pci.has_value()) {
    trp_info_type_resp_item_c item;
    item.set_pci_nr() = trp.pci.value();
    asn1out.trp_info_type_resp_list.push_back(item);
  }
  if (trp.cgi.has_value()) {
    trp_info_type_resp_item_c item;
    item.set_ng_ran_cgi().nr_cell_id.from_number(trp.cgi.value().nci.value());
    item.ng_ran_cgi().plmn_id = trp.cgi.value().plmn_id.to_bytes();
    asn1out.trp_info_type_resp_list.push_back(item);
  }
  if (trp.arfcn.has_value()) {
    trp_info_type_resp_item_c item;
    item.set_nr_arfcn() = trp.arfcn.value();
    asn1out.trp_info_type_resp_list.push_back(item);
  }

  // TODO: Remaining

  return asn1out;
}

static srs_config::srs_resource from_asn1(const srs_res_s& asn1cfg)
{
  srs_config::srs_resource out{};

  out.id.ue_res_id = static_cast<srs_config::srs_res_id>(asn1cfg.srs_res_id);
  out.nof_ports    = static_cast<srs_config::srs_resource::nof_srs_ports>(asn1cfg.nrof_srs_ports.to_number());
  if (asn1cfg.tx_comb.type().value == tx_comb_c::types_opts::n2) {
    const auto& n2                   = asn1cfg.tx_comb.n2();
    out.tx_comb.size                 = tx_comb_size::n2;
    out.tx_comb.tx_comb_offset       = n2.comb_offset_n2;
    out.tx_comb.tx_comb_cyclic_shift = n2.cyclic_shift_n2;
  } else {
    const auto& n4                   = asn1cfg.tx_comb.n4();
    out.tx_comb.size                 = tx_comb_size::n4;
    out.tx_comb.tx_comb_offset       = n4.comb_offset_n4;
    out.tx_comb.tx_comb_cyclic_shift = n4.cyclic_shift_n4;
  }
  out.res_mapping.start_pos   = asn1cfg.start_position;
  out.res_mapping.nof_symb    = static_cast<srs_nof_symbols>(asn1cfg.nrof_symbols.to_number());
  out.res_mapping.rept_factor = static_cast<srs_nof_symbols>(asn1cfg.repeat_factor.to_number());
  out.freq_domain_pos         = asn1cfg.freq_domain_position;
  out.freq_domain_shift       = asn1cfg.freq_domain_shift;
  out.freq_hop.c_srs          = asn1cfg.c_srs;
  out.freq_hop.b_srs          = asn1cfg.b_srs;
  out.freq_hop.b_hop          = asn1cfg.b_hop;
  switch (asn1cfg.group_or_seq_hop) {
    case srs_res_s::group_or_seq_hop_opts::neither:
      out.grp_or_seq_hop = srs_group_or_sequence_hopping::neither;
      break;
    case srs_res_s::group_or_seq_hop_opts::group_hop:
      out.grp_or_seq_hop = srs_group_or_sequence_hopping::group_hopping;
      break;
    case srs_res_s::group_or_seq_hop_opts::seq_hop:
      out.grp_or_seq_hop = srs_group_or_sequence_hopping::sequence_hopping;
      break;
    default:
      report_fatal_error("Invalid grp or seq hop={}", fmt::underlying(asn1cfg.group_or_seq_hop.value));
  }
  switch (asn1cfg.res_type.type().value) {
    case res_type_c::types_opts::aperiodic: {
      out.res_type = srs_resource_type::aperiodic;
    } break;
    case res_type_c::types_opts::semi_persistent: {
      out.res_type = srs_resource_type::semi_persistent;
      out.periodicity_and_offset.emplace();
      out.periodicity_and_offset.value().period =
          static_cast<srs_periodicity>(asn1cfg.res_type.semi_persistent().periodicity.to_number());
      out.periodicity_and_offset.value().offset = asn1cfg.res_type.semi_persistent().offset;
    } break;
    case res_type_c::types_opts::periodic: {
      out.res_type = srs_resource_type::periodic;
      out.periodicity_and_offset.emplace();
      out.periodicity_and_offset.value().period =
          static_cast<srs_periodicity>(asn1cfg.res_type.periodic().periodicity.to_number());
      out.periodicity_and_offset.value().offset = asn1cfg.res_type.periodic().offset;
    } break;
    default:
      ocudu_assertion_failure("Invalid resource type={}", asn1cfg.res_type.type().to_string());
  }
  out.sequence_id = asn1cfg.seq_id;

  return out;
}

static srs_config::srs_resource_set from_asn1(const srs_res_set_s& asn1cfg)
{
  srs_config::srs_resource_set out{};

  out.id = static_cast<srs_config::srs_res_set_id>(asn1cfg.srs_res_set_id);
  for (const auto& res_id : asn1cfg.srs_res_id_list) {
    out.srs_res_id_list.push_back(static_cast<srs_config::srs_res_id>(res_id));
  }
  switch (asn1cfg.res_set_type.type().value) {
    case res_set_type_c::types_opts::periodic:
      out.res_type.emplace<srs_config::srs_resource_set::periodic_resource_type>();
      break;
    case res_set_type_c::types_opts::aperiodic: {
      auto& aperiodic = out.res_type.emplace<srs_config::srs_resource_set::aperiodic_resource_type>();
      aperiodic.aperiodic_srs_res_trigger = asn1cfg.res_set_type.aperiodic().srs_res_trigger_list;
      aperiodic.slot_offset               = asn1cfg.res_set_type.aperiodic().slotoffset;
    } break;
    case res_set_type_c::types_opts::semi_persistent:
      out.res_type.emplace<srs_config::srs_resource_set::semi_persistent_resource_type>();
      break;
    default:
      report_fatal_error("Invalid resource set type");
  }

  return out;
}

static srs_config from_asn1(const srs_cfg_s& asn1cfg)
{
  srs_config outcfg;
  outcfg.srs_res_list.resize(asn1cfg.srs_res_list.size());
  for (unsigned i = 0, ie = outcfg.srs_res_list.size(); i != ie; ++i) {
    outcfg.srs_res_list[i] = from_asn1(asn1cfg.srs_res_list[i]);
  }
  outcfg.srs_res_set_list.resize(asn1cfg.srs_res_set_list.size());
  for (unsigned i = 0, ie = outcfg.srs_res_set_list.size(); i != ie; ++i) {
    outcfg.srs_res_set_list[i] = from_asn1(asn1cfg.srs_res_set_list[i]);
  }
  return outcfg;
}

std::vector<srs_carrier> asn1_helper::from_asn1(const asn1::f1ap::srs_configuration_s& asn1cfg)
{
  std::vector<srs_carrier> out;

  out.resize(asn1cfg.srs_carrier_list.size());
  for (unsigned i = 0, e = out.size(); i != e; ++i) {
    const auto& asn1item              = asn1cfg.srs_carrier_list[i];
    auto&       outitem               = out[i];
    outitem.point_a                   = asn1item.point_a;
    outitem.freq_shift_7p5khz_present = asn1item.active_ul_bwp.shift7dot5k_hz_present;
    outitem.ul_ch_bw_per_scs_list.resize(asn1item.ul_ch_bw_per_scs_list.size());
    for (unsigned j = 0, je = outitem.ul_ch_bw_per_scs_list.size(); j != je; ++j) {
      const auto& asn1bw      = asn1item.ul_ch_bw_per_scs_list[j];
      auto&       outbw       = outitem.ul_ch_bw_per_scs_list[j];
      outbw.offset_to_carrier = asn1bw.offset_to_carrier;
      outbw.scs               = static_cast<subcarrier_spacing>(asn1bw.subcarrier_spacing.value);
      outbw.carrier_bandwidth = asn1bw.carrier_bw;
    }
    // UL BWP Cfg
    outitem.ul_bwp_cfg.cp  = asn1item.active_ul_bwp.cp.value == active_ul_bwp_s::cp_opts::normal
                                 ? cyclic_prefix::NORMAL
                                 : cyclic_prefix::EXTENDED;
    outitem.ul_bwp_cfg.scs = static_cast<subcarrier_spacing>(asn1item.active_ul_bwp.subcarrier_spacing.value);
    unsigned start_rb, len_rb;
    sliv_to_s_and_l(MAX_NOF_PRBS, asn1item.active_ul_bwp.location_and_bw, start_rb, len_rb);
    outitem.ul_bwp_cfg.crbs = {start_rb, start_rb + len_rb};
    // SRSConfig
    outitem.srs_cfg = ::from_asn1(asn1item.active_ul_bwp.srs_cfg);
    // PCI
    if (asn1item.pci_present) {
      out[i].pci = asn1item.pci;
    }
  }

  return out;
}
