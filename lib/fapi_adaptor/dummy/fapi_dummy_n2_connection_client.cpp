// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "fapi_dummy_n2_connection_client.h"
#include "ocudu/asn1/ngap/common.h"
#include "ocudu/asn1/ngap/ngap_pdu_contents.h"
#include "ocudu/ngap/ngap_message.h"
#include "ocudu/pcap/dlt_pcap.h"
#include "ocudu/support/ocudu_assert.h"
#include <map>

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace ocudu::fapi_adaptor;

namespace {

class fapi_dummy_no_core_amf_stub final : public ocucp::n2_connection_client
{
public:
  explicit fapi_dummy_no_core_amf_stub(dlt_pcap& pcap_) : pcap_writer(pcap_) {}

  std::unique_ptr<ocucp::ngap_message_notifier>
  handle_cu_cp_connection_request(std::unique_ptr<ocucp::ngap_rx_message_notifier> cu_cp_rx_pdu_notifier) override
  {
    class cu_cp_tx_pdu_notifier final : public ocucp::ngap_message_notifier
    {
    public:
      cu_cp_tx_pdu_notifier(fapi_dummy_no_core_amf_stub& parent_) : parent(parent_) {}
      ~cu_cp_tx_pdu_notifier() { parent.disconnect(); }

      [[nodiscard]] bool on_new_message(const ngap_message& msg) override
      {
        parent.handle_tx_message(msg);
        return true;
      }

    private:
      fapi_dummy_no_core_amf_stub& parent;
    };

    cu_cp_rx_notifier = std::move(cu_cp_rx_pdu_notifier);
    return std::make_unique<cu_cp_tx_pdu_notifier>(*this);
  }

private:
  void disconnect() { cu_cp_rx_notifier.reset(); }

  void handle_tx_message(const ngap_message& msg)
  {
    using namespace asn1::ngap;

    if (pcap_writer.is_write_enabled()) {
      byte_buffer   packed_pdu{byte_buffer::fallback_allocation_tag{}};
      asn1::bit_ref bref{packed_pdu};
      if (msg.pdu.pack(bref) == asn1::OCUDUASN_SUCCESS) {
        pcap_writer.push_pdu(std::move(packed_pdu));
      }
    }

    if (msg.pdu.type().value != ngap_pdu_c::types_opts::init_msg) {
      return;
    }

    const auto msg_type = msg.pdu.init_msg().value.type().value;

    if (msg_type == ngap_elem_procs_o::init_msg_c::types_opts::ng_setup_request) {
      const auto& req           = msg.pdu.init_msg().value.ng_setup_request();
      ocudu_assert(req->supported_ta_list.size() > 0, "NG Setup Request has no supported TA list");
      const auto& broadcast_plmns = req->supported_ta_list[0].broadcast_plmn_list;

      if (broadcast_plmns.size() > 0) {
        saved_plmn       = broadcast_plmns[0].plmn_id;
        saved_slice_list = broadcast_plmns[0].tai_slice_support_list;
      }

      ngap_message resp;
      resp.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_NG_SETUP);
      auto& ng_resp = resp.pdu.successful_outcome().value.ng_setup_resp();
      ng_resp->amf_name.from_string("localamf");
      ng_resp->served_guami_list.resize(broadcast_plmns.size());
      for (unsigned i = 0; i != ng_resp->served_guami_list.size(); ++i) {
        ng_resp->served_guami_list[i].guami.plmn_id = broadcast_plmns[i].plmn_id;
        ng_resp->served_guami_list[i].guami.amf_region_id.from_number(0x2);
        ng_resp->served_guami_list[i].guami.amf_set_id.from_number(0x40);
        ng_resp->served_guami_list[i].guami.amf_pointer.from_number(0x0);
      }
      ng_resp->relative_amf_capacity = 255;
      ng_resp->plmn_support_list.resize(broadcast_plmns.size());
      for (unsigned i = 0; i != broadcast_plmns.size(); ++i) {
        auto& out_plmn              = ng_resp->plmn_support_list[i];
        out_plmn.plmn_id            = broadcast_plmns[i].plmn_id;
        out_plmn.slice_support_list = broadcast_plmns[i].tai_slice_support_list;
      }
      send_to_cu_cp(resp);

    } else if (msg_type == ngap_elem_procs_o::init_msg_c::types_opts::init_ue_msg) {
      const auto& req       = msg.pdu.init_msg().value.init_ue_msg();
      const auto  ran_ue_id = req->ran_ue_ngap_id;
      const auto  amf_ue_id = next_amf_ue_id++;
      ran_to_amf_map[ran_ue_id] = amf_ue_id;

      ngap_message ics;
      ics.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_INIT_CONTEXT_SETUP);
      auto& r           = ics.pdu.init_msg().value.init_context_setup_request();
      r->amf_ue_ngap_id = amf_ue_id;
      r->ran_ue_ngap_id = ran_ue_id;

      r->guami.plmn_id = saved_plmn;
      r->guami.amf_region_id.from_number(0x2);
      r->guami.amf_set_id.from_number(0x40);
      r->guami.amf_pointer.from_number(0x0);

      r->nas_pdu_present = true;
      r->nas_pdu.from_string(
          "7e02c4f6c22f017e0042010177000bf202f8998000410000001054070002f8990000011500210201005e01b6");

      r->ue_security_cap.nr_encryption_algorithms.from_number(0);
      r->ue_security_cap.nr_integrity_protection_algorithms.from_number(49152);
      r->ue_security_cap.eutr_aencryption_algorithms.from_number(0);
      r->ue_security_cap.eutr_aintegrity_protection_algorithms.from_number(0);

      r->security_key.from_string(
          "1111111111111111111111111111111111111111111111111111111111111111"
          "1111111111111111111111111111111111111111111111111111111111111111"
          "1111111111111111111111111111111111111111111111111111111111111111"
          "1111111111111111111111111111111111111111111111111111111111111111");

      if (saved_slice_list.size() > 0) {
        allowed_nssai_item_s item{};
        item.s_nssai.sst         = saved_slice_list[0].s_nssai.sst;
        item.s_nssai.sd_present  = saved_slice_list[0].s_nssai.sd_present;
        item.s_nssai.sd          = saved_slice_list[0].s_nssai.sd;
        r->allowed_nssai.push_back(item);
      } else {
        allowed_nssai_item_s item{};
        item.s_nssai.sst.from_number(1);
        r->allowed_nssai.push_back(item);
      }

      r->ue_radio_cap_present = true;
      r->ue_radio_cap.from_string(
          "02c0856c18033a047465a025f800082a00241260e000307838031bff001300098a00d00000001c058d006007a071e439f0000240400e"
          "03000000001001be00000000000002074000000000005038a12007000f00000004008010");

      // UE aggregate max bit rate — required by CU-UP when PDU sessions are present.
      r->ue_aggr_max_bit_rate_present                      = true;
      r->ue_aggr_max_bit_rate.ue_aggr_max_bit_rate_dl      = 300000000U; // 300 Mbps
      r->ue_aggr_max_bit_rate.ue_aggr_max_bit_rate_ul      = 200000000U; // 200 Mbps

      // PDU session triggers DRB setup (RRCReconfiguration) after SecurityModeComplete.
      r->pdu_session_res_setup_list_cxt_req_present = true;
      pdu_session_res_setup_item_cxt_req_s pdu_session_item{};
      pdu_session_item.pdu_session_id = 1U;
      pdu_session_item.s_nssai.sst.from_number(1U);
      pdu_session_item.pdu_session_res_setup_request_transfer.from_string(
          "0000040082000a0c400000003040000000008b000a01f00a3213020000049a00860001000088000700010000091c00");
      r->pdu_session_res_setup_list_cxt_req.push_back(pdu_session_item);

      send_to_cu_cp(ics);

    } else if (msg_type == ngap_elem_procs_o::init_msg_c::types_opts::ue_context_release_request) {
      // Absorb silently — no_core mode keeps UEs connected indefinitely.
    }
  }

  void send_to_cu_cp(const ngap_message& msg)
  {
    ocudu_assert(cu_cp_rx_notifier != nullptr, "Adapter is disconnected");

    if (pcap_writer.is_write_enabled()) {
      byte_buffer         bytes;
      asn1::bit_ref       bref{bytes};
      if (msg.pdu.pack(bref) == asn1::OCUDUASN_SUCCESS) {
        pcap_writer.push_pdu(std::move(bytes));
      }
    }

    cu_cp_rx_notifier->on_new_message(msg);
  }

  dlt_pcap& pcap_writer;

  std::unique_ptr<ocucp::ngap_rx_message_notifier> cu_cp_rx_notifier;

  uint64_t                             next_amf_ue_id = 1;
  std::map<uint64_t, uint64_t>         ran_to_amf_map;
  asn1::fixed_octstring<3, true>       saved_plmn;
  asn1::ngap::slice_support_list_l     saved_slice_list;
};

} // namespace

std::unique_ptr<ocucp::n2_connection_client>
ocudu::fapi_adaptor::create_fapi_dummy_n2_connection_client(dlt_pcap& pcap)
{
  return std::make_unique<fapi_dummy_no_core_amf_stub>(pcap);
}
