/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "f1ap_test_messages.h"
#include "ocudu/asn1/f1ap/common.h"
#include "ocudu/asn1/f1ap/f1ap_ies.h"
#include "ocudu/asn1/f1ap/f1ap_pdu_contents.h"
#include "ocudu/f1ap/f1ap_message.h"
#include "ocudu/ran/positioning/positioning_ids.h"

using namespace ocudu;
using namespace asn1::f1ap;

f1ap_message ocudu::test_helpers::generate_trp_information_request()
{
  f1ap_message msg;

  msg.pdu.set_init_msg().load_info_obj(ASN1_F1AP_ID_TRP_INFO_EXCHANGE);
  auto& req = msg.pdu.init_msg().value.trp_info_request();

  req->transaction_id = 0;

  std::vector<asn1::f1ap::trp_info_type_item_opts::options> trp_info_type_list = {
      asn1::f1ap::trp_info_type_item_opts::options::nr_pci,
      asn1::f1ap::trp_info_type_item_opts::options::ng_ran_cgi,
      asn1::f1ap::trp_info_type_item_opts::options::arfcn};

  for (const auto& trp_info_type_item : trp_info_type_list) {
    asn1::protocol_ie_single_container_s<asn1::f1ap::trp_info_type_item_trp_req_o> trp_info_type_item_container;
    trp_info_type_item_container->trp_info_type_item() = trp_info_type_item;

    req->trp_info_type_list_trp_req.push_back(trp_info_type_item_container);
  }

  return msg;
}

f1ap_message ocudu::test_helpers::generate_trp_information_response(const std::vector<trp_id_t>& trp_ids)
{
  f1ap_message pdu = {};

  pdu.pdu.set_successful_outcome();
  pdu.pdu.successful_outcome().load_info_obj(ASN1_F1AP_ID_TRP_INFO_EXCHANGE);

  auto& trp_info_resp           = pdu.pdu.successful_outcome().value.trp_info_resp();
  trp_info_resp->transaction_id = 0;

  for (const auto& trp_id : trp_ids) {
    unsigned pci         = 1;
    unsigned nr_cgi_uint = 6576;

    asn1::protocol_ie_single_container_s<asn1::f1ap::trp_info_item_trp_resp_o> trp_resp_item_container;
    asn1::f1ap::trp_info_item_s& trp_info_item = trp_resp_item_container->trp_info_item();
    trp_info_item.trp_info.trp_id              = trp_id_to_uint(trp_id);

    // Add PCI.
    asn1::f1ap::trp_info_type_resp_item_c trp_info_type_resp_item;
    trp_info_type_resp_item.set_pci_nr() = pci;
    trp_info_item.trp_info.trp_info_type_resp_list.push_back(trp_info_type_resp_item);

    // Add NR CGI.
    asn1::f1ap::trp_info_type_resp_item_c trp_info_type_resp_item_2;
    asn1::f1ap::nr_cgi_s                  asn1_cgi;
    asn1_cgi.plmn_id.from_string("00f110");
    asn1_cgi.nr_cell_id.from_number(nr_cgi_uint);
    trp_info_type_resp_item_2.set_ng_ran_cgi() = asn1_cgi;
    trp_info_item.trp_info.trp_info_type_resp_list.push_back(trp_info_type_resp_item_2);

    // Add ARFCN.
    asn1::f1ap::trp_info_type_resp_item_c trp_info_type_resp_item_3;
    trp_info_type_resp_item_3.set_nr_arfcn() = 652054;
    trp_info_item.trp_info.trp_info_type_resp_list.push_back(trp_info_type_resp_item_3);

    trp_info_resp->trp_info_list_trp_resp.push_back(trp_resp_item_container);

    ++pci;
    ++nr_cgi_uint;
  }

  return pdu;
}

f1ap_message ocudu::test_helpers::generate_trp_information_failure()
{
  f1ap_message pdu = {};

  pdu.pdu.set_unsuccessful_outcome();
  pdu.pdu.unsuccessful_outcome().load_info_obj(ASN1_F1AP_ID_TRP_INFO_EXCHANGE);

  auto& trp_info_fail                      = pdu.pdu.unsuccessful_outcome().value.trp_info_fail();
  trp_info_fail->transaction_id            = 0;
  trp_info_fail->cause.set_radio_network() = cause_radio_network_e::unspecified;

  return pdu;
}

f1ap_message ocudu::test_helpers::generate_positioning_information_request(gnb_du_ue_f1ap_id_t du_ue_id,
                                                                           gnb_cu_ue_f1ap_id_t cu_ue_id)
{
  f1ap_message pdu = {};

  pdu.pdu.set_init_msg();
  pdu.pdu.init_msg().load_info_obj(ASN1_F1AP_ID_POSITIONING_INFO_EXCHANGE);

  auto& pos_info_req                                             = pdu.pdu.init_msg().value.positioning_info_request();
  pos_info_req->gnb_cu_ue_f1ap_id                                = gnb_cu_ue_f1ap_id_to_uint(cu_ue_id);
  pos_info_req->gnb_du_ue_f1ap_id                                = gnb_du_ue_f1ap_id_to_uint(du_ue_id);
  pos_info_req->requested_srs_tx_characteristics_present         = true;
  pos_info_req->requested_srs_tx_characteristics.nof_txs_present = true;
  pos_info_req->requested_srs_tx_characteristics.nof_txs         = 0;
  pos_info_req->requested_srs_tx_characteristics.res_type =
      asn1::f1ap::requested_srs_tx_characteristics_s::res_type_opts::options::periodic;
  pos_info_req->requested_srs_tx_characteristics.bw_srs.set_fr1() = asn1::f1ap::fr1_bw_opts::options::bw100;

  return pdu;
}

inline asn1::f1ap::srs_configuration_s generate_srs_configuration(subcarrier_spacing scs    = subcarrier_spacing::kHz15,
                                                                  unsigned           offset = 0U)
{
  asn1::f1ap::srs_configuration_s srs_configuration;

  asn1::f1ap::srs_carrier_list_item_s srs_carrier_item;
  srs_carrier_item.point_a = 632016;

  ocudu_assert(scs <= subcarrier_spacing::kHz30 or offset == 0U,
               "Only SCS 15 kHz and 30kHz SCSs are currently supported in this test");

  // Fill UL CH BW per SCS list.
  scs_specific_carrier_s scs_specific_carrier;
  scs_specific_carrier.offset_to_carrier = 0;
  scs_specific_carrier.subcarrier_spacing =
      scs == ocudu::subcarrier_spacing::kHz15
          ? asn1::f1ap::scs_specific_carrier_s::subcarrier_spacing_opts::options::khz15
          : asn1::f1ap::scs_specific_carrier_s::subcarrier_spacing_opts::options::khz30;
  scs_specific_carrier.carrier_bw = 52;

  srs_carrier_item.ul_ch_bw_per_scs_list.push_back(scs_specific_carrier);

  // Fill Active UL BWP.
  srs_carrier_item.active_ul_bwp.location_and_bw = 13750;
  srs_carrier_item.active_ul_bwp.subcarrier_spacing =
      asn1::f1ap::active_ul_bwp_s::subcarrier_spacing_opts::options::khz15;
  srs_carrier_item.active_ul_bwp.cp                         = asn1::f1ap::active_ul_bwp_s::cp_opts::options::normal;
  srs_carrier_item.active_ul_bwp.tx_direct_current_location = 3300;

  asn1::f1ap::srs_res_s srs_res;
  srs_res.srs_res_id       = 0;
  srs_res.nrof_srs_ports   = asn1::f1ap::srs_res_s::nrof_srs_ports_opts::options::port1;
  srs_res.tx_comb.set_n4() = asn1::f1ap::tx_comb_c::n4_s_{
      .comb_offset_n4  = 0,
      .cyclic_shift_n4 = 0,
  };
  srs_res.start_position        = 0;
  srs_res.nrof_symbols          = asn1::f1ap::srs_res_s::nrof_symbols_opts::options::n1;
  srs_res.repeat_factor         = asn1::f1ap::srs_res_s::repeat_factor_opts::options::n1;
  srs_res.freq_domain_position  = 0;
  srs_res.freq_domain_shift     = 0;
  srs_res.c_srs                 = 14;
  srs_res.b_srs                 = 0;
  srs_res.b_hop                 = 0;
  srs_res.group_or_seq_hop      = asn1::f1ap::srs_res_s::group_or_seq_hop_opts::options::neither;
  auto& periodic_res_type       = srs_res.res_type.set_periodic();
  periodic_res_type.periodicity = asn1::f1ap::res_type_periodic_s::periodicity_opts::options::slot80;
  periodic_res_type.offset      = offset;
  srs_carrier_item.active_ul_bwp.srs_cfg.srs_res_list.push_back(srs_res);

  asn1::f1ap::srs_res_set_s srs_res_set;
  srs_res_set.srs_res_set_id = 0U;
  srs_res_set.srs_res_id_list.push_back(srs_res.srs_res_id);
  auto& periodic_res_set_type        = srs_res_set.res_set_type.set_periodic();
  periodic_res_set_type.periodic_set = asn1::f1ap::res_set_type_periodic_s::periodic_set_opts::options::true_value;

  srs_carrier_item.active_ul_bwp.srs_cfg.srs_res_set_list.push_back(srs_res_set);

  srs_carrier_item.pci_present = false;

  srs_configuration.srs_carrier_list.push_back(srs_carrier_item);

  return srs_configuration;
}

f1ap_message ocudu::test_helpers::generate_positioning_information_response(gnb_du_ue_f1ap_id_t du_ue_id,
                                                                            gnb_cu_ue_f1ap_id_t cu_ue_id)
{
  f1ap_message pdu = {};

  pdu.pdu.set_successful_outcome();
  pdu.pdu.successful_outcome().load_info_obj(ASN1_F1AP_ID_POSITIONING_INFO_EXCHANGE);

  auto& pos_info_resp              = pdu.pdu.successful_outcome().value.positioning_info_resp();
  pos_info_resp->gnb_cu_ue_f1ap_id = gnb_cu_ue_f1ap_id_to_uint(cu_ue_id);
  pos_info_resp->gnb_du_ue_f1ap_id = gnb_du_ue_f1ap_id_to_uint(du_ue_id);

  pos_info_resp->srs_configuration_present = true;
  pos_info_resp->srs_configuration         = generate_srs_configuration();

  return pdu;
}

f1ap_message ocudu::test_helpers::generate_positioning_information_failure(gnb_du_ue_f1ap_id_t du_ue_id,
                                                                           gnb_cu_ue_f1ap_id_t cu_ue_id)
{
  f1ap_message pdu = {};

  pdu.pdu.set_unsuccessful_outcome();
  pdu.pdu.unsuccessful_outcome().load_info_obj(ASN1_F1AP_ID_POSITIONING_INFO_EXCHANGE);

  auto& pos_info_fail                      = pdu.pdu.unsuccessful_outcome().value.positioning_info_fail();
  pos_info_fail->gnb_cu_ue_f1ap_id         = gnb_cu_ue_f1ap_id_to_uint(cu_ue_id);
  pos_info_fail->gnb_du_ue_f1ap_id         = gnb_du_ue_f1ap_id_to_uint(du_ue_id);
  pos_info_fail->cause.set_radio_network() = cause_radio_network_e::unspecified;

  return pdu;
}

f1ap_message ocudu::test_helpers::generate_positioning_activation_response(gnb_du_ue_f1ap_id_t du_ue_id,
                                                                           gnb_cu_ue_f1ap_id_t cu_ue_id)
{
  f1ap_message pdu = {};

  pdu.pdu.set_successful_outcome();
  pdu.pdu.successful_outcome().load_info_obj(ASN1_F1AP_ID_POSITIONING_ACTIVATION);

  auto& pos_info_resp              = pdu.pdu.successful_outcome().value.positioning_activation_resp();
  pos_info_resp->gnb_cu_ue_f1ap_id = gnb_cu_ue_f1ap_id_to_uint(cu_ue_id);
  pos_info_resp->gnb_du_ue_f1ap_id = gnb_du_ue_f1ap_id_to_uint(du_ue_id);

  return pdu;
}

f1ap_message ocudu::test_helpers::generate_positioning_activation_failure(gnb_du_ue_f1ap_id_t du_ue_id,
                                                                          gnb_cu_ue_f1ap_id_t cu_ue_id)
{
  f1ap_message pdu = {};

  pdu.pdu.set_unsuccessful_outcome();
  pdu.pdu.unsuccessful_outcome().load_info_obj(ASN1_F1AP_ID_POSITIONING_ACTIVATION);

  auto& pos_info_fail                      = pdu.pdu.unsuccessful_outcome().value.positioning_activation_fail();
  pos_info_fail->gnb_cu_ue_f1ap_id         = gnb_cu_ue_f1ap_id_to_uint(cu_ue_id);
  pos_info_fail->gnb_du_ue_f1ap_id         = gnb_du_ue_f1ap_id_to_uint(du_ue_id);
  pos_info_fail->cause.set_radio_network() = cause_radio_network_e::unspecified;

  return pdu;
}

f1ap_message ocudu::test_helpers::generate_positioning_measurement_request(
    std::vector<trp_id_t>                                trp_ids,
    lmf_meas_id_t                                        lmf_meas_id,
    ran_meas_id_t                                        ran_meas_id,
    std::vector<asn1::f1ap::pos_meas_type_opts::options> pos_meas_type_list,
    subcarrier_spacing                                   scs,
    unsigned                                             srs_offset)
{
  f1ap_message pdu = {};

  pdu.pdu.set_init_msg();
  pdu.pdu.init_msg().load_info_obj(ASN1_F1AP_ID_POSITIONING_MEAS_EXCHANGE);

  auto& pos_meas_req           = pdu.pdu.init_msg().value.positioning_meas_request();
  pos_meas_req->transaction_id = 1;
  pos_meas_req->lmf_meas_id    = lmf_meas_id_to_uint(lmf_meas_id);
  pos_meas_req->ran_meas_id    = ran_meas_id_to_uint(ran_meas_id);

  for (const auto trp_id : trp_ids) {
    pos_meas_req->trp_meas_request_list.push_back(
        asn1::f1ap::trp_meas_request_item_s{.trp_id = trp_id_to_uint(trp_id)});
  }

  pos_meas_req->pos_report_characteristics = asn1::f1ap::pos_report_characteristics_opts::options::ondemand;

  for (const auto& pos_meas_type : pos_meas_type_list) {
    asn1::f1ap::pos_meas_quantities_item_s meas_quantities_item;
    meas_quantities_item.pos_meas_type = pos_meas_type;
    if (pos_meas_type == asn1::f1ap::pos_meas_type_opts::options::ul_rtoa) {
      meas_quantities_item.timing_report_granularity_factor_present = true;
      meas_quantities_item.timing_report_granularity_factor         = 1;
    }

    pos_meas_req->pos_meas_quantities.push_back(meas_quantities_item);
  }

  pos_meas_req->srs_configuration_present = true;
  pos_meas_req->srs_configuration         = generate_srs_configuration(scs, srs_offset);

  return pdu;
}

f1ap_message ocudu::test_helpers::generate_positioning_measurement_response(lmf_meas_id_t                lmf_meas_id,
                                                                            ran_meas_id_t                ran_meas_id,
                                                                            const std::vector<trp_id_t>& trp_ids,
                                                                            unsigned                     transaction_id)
{
  f1ap_message pdu = {};

  pdu.pdu.set_successful_outcome();
  pdu.pdu.successful_outcome().load_info_obj(ASN1_F1AP_ID_POSITIONING_MEAS_EXCHANGE);

  auto& pos_meas_resp           = pdu.pdu.successful_outcome().value.positioning_meas_resp();
  pos_meas_resp->transaction_id = transaction_id;
  pos_meas_resp->lmf_meas_id    = lmf_meas_id_to_uint(lmf_meas_id);
  pos_meas_resp->ran_meas_id    = ran_meas_id_to_uint(ran_meas_id);

  for (const auto& trp_id : trp_ids) {
    // Create positioning measurement result item for each TRP ID.
    asn1::f1ap::pos_meas_result_list_item_s meas_resp_item;
    meas_resp_item.trp_id = trp_id_to_uint(trp_id);

    // Add positioning measurement result item.
    asn1::f1ap::pos_meas_result_item_s pos_meas_result_item;

    // > Add UL RTOA measurement result.
    asn1::f1ap::ul_rtoa_meas_s& ul_rtoa = pos_meas_result_item.measured_results_value.set_ul_rtoa();
    ul_rtoa.ul_rtoa_meas_item.set_k0()  = 985471;

    // Add time stamp.
    pos_meas_result_item.time_stamp.sys_frame_num         = 25;
    pos_meas_result_item.time_stamp.slot_idx.set_scs_30() = 0;

    // Add measurement quality.
    pos_meas_result_item.meas_quality_present = true;
    asn1::f1ap::timing_meas_quality_s& timing_meas_qualitiy =
        pos_meas_result_item.meas_quality.tr_pmeas_quality_item.set_timing_meas_quality();
    timing_meas_qualitiy.meas_quality = 0;
    timing_meas_qualitiy.resolution   = asn1::f1ap::timing_meas_quality_s::resolution_opts::options::m0dot1;

    meas_resp_item.pos_meas_result.push_back(pos_meas_result_item);

    pos_meas_resp->pos_meas_result_list_present = true;
    pos_meas_resp->pos_meas_result_list.push_back(meas_resp_item);
  }

  return pdu;
}

f1ap_message ocudu::test_helpers::generate_positioning_measurement_failure(lmf_meas_id_t lmf_meas_id,
                                                                           ran_meas_id_t ran_meas_id)
{
  f1ap_message pdu = {};

  pdu.pdu.set_unsuccessful_outcome();
  pdu.pdu.unsuccessful_outcome().load_info_obj(ASN1_F1AP_ID_POSITIONING_MEAS_EXCHANGE);

  auto& pos_meas_fail                      = pdu.pdu.unsuccessful_outcome().value.positioning_meas_fail();
  pos_meas_fail->transaction_id            = 1;
  pos_meas_fail->lmf_meas_id               = lmf_meas_id_to_uint(lmf_meas_id);
  pos_meas_fail->ran_meas_id               = ran_meas_id_to_uint(ran_meas_id);
  pos_meas_fail->cause.set_radio_network() = cause_radio_network_e::unspecified;

  return pdu;
}
