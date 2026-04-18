// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "ocudu/ngap/gateways/n2_connection_client_factory.h"
#include "ocudu/asn1/ngap/common.h"
#include "ocudu/asn1/ngap/ngap_pdu_contents.h"
#include "ocudu/gateways/sctp_network_client_factory.h"
#include "ocudu/ngap/ngap_message.h"
#include "ocudu/pcap/dlt_pcap.h"
#include <map>

using namespace ocudu;
using namespace ocucp;

namespace {

/// Notifier for converting packed NGAP PDUs coming from the N2 GW into unpacked NGAP PDUs and forward them to the
/// CU-CP.
class sctp_to_n2_pdu_notifier final : public sctp_association_sdu_notifier
{
public:
  sctp_to_n2_pdu_notifier(std::unique_ptr<ngap_rx_message_notifier> cu_cp_rx_pdu_notifier_,
                          dlt_pcap&                                 pcap_writer_,
                          ocudulog::basic_logger&                   logger_) :
    cu_cp_rx_pdu_notifier(std::move(cu_cp_rx_pdu_notifier_)), pcap_writer(pcap_writer_), logger(logger_)
  {
  }

  bool on_new_sdu(byte_buffer sdu) override
  {
    // Unpack NGAP PDU.
    asn1::cbit_ref bref(sdu);
    ngap_message   msg;
    if (msg.pdu.unpack(bref) != asn1::OCUDUASN_SUCCESS) {
      logger.error("Couldn't unpack NGAP PDU");
      return false;
    }

    // Forward Rx PDU to pcap, if enabled.
    if (pcap_writer.is_write_enabled()) {
      pcap_writer.push_pdu(sdu.copy());
    }

    // Forward unpacked Rx PDU to the CU-CP.
    cu_cp_rx_pdu_notifier->on_new_message(msg);
    return true;
  }

private:
  std::unique_ptr<ngap_rx_message_notifier> cu_cp_rx_pdu_notifier;
  dlt_pcap&                                 pcap_writer;
  ocudulog::basic_logger&                   logger;
};

/// \brief Notifier for converting unpacked NGAP PDUs coming from the CU-CP into packed NGAP PDUs and forward them to
/// the N2 GW.
class n2_to_sctp_pdu_notifier final : public ngap_message_notifier
{
public:
  n2_to_sctp_pdu_notifier(std::unique_ptr<sctp_association_sdu_notifier> sctp_rx_pdu_notifier_,
                          dlt_pcap&                                      pcap_writer_,
                          ocudulog::basic_logger&                        logger_) :
    sctp_rx_pdu_notifier(std::move(sctp_rx_pdu_notifier_)), pcap_writer(pcap_writer_), logger(logger_)
  {
  }

  [[nodiscard]] bool on_new_message(const ngap_message& msg) override
  {
    // pack NGAP PDU into SCTP SDU.
    byte_buffer   tx_sdu{byte_buffer::fallback_allocation_tag{}};
    asn1::bit_ref bref(tx_sdu);
    if (msg.pdu.pack(bref) != asn1::OCUDUASN_SUCCESS) {
      logger.error("Failed to pack NGAP PDU");
      return false;
    }

    // Push Tx PDU to pcap.
    if (pcap_writer.is_write_enabled()) {
      pcap_writer.push_pdu(tx_sdu.copy());
    }

    // Forward packed Tx PDU to SCTP gateway.
    sctp_rx_pdu_notifier->on_new_sdu(std::move(tx_sdu));

    return true;
  }

private:
  std::unique_ptr<sctp_association_sdu_notifier> sctp_rx_pdu_notifier;
  dlt_pcap&                                      pcap_writer;
  ocudulog::basic_logger&                        logger;
};

/// Stub for the operation of the CU-CP without a core.
class ngap_gateway_local_stub final : public n2_connection_client
{
public:
  explicit ngap_gateway_local_stub(dlt_pcap& pcap_) : pcap_writer(pcap_) {}

  std::unique_ptr<ngap_message_notifier>
  handle_cu_cp_connection_request(std::unique_ptr<ngap_rx_message_notifier> cu_cp_rx_pdu_notifier) override
  {
    class cu_cp_tx_pdu_notifier final : public ngap_message_notifier
    {
    public:
      cu_cp_tx_pdu_notifier(ngap_gateway_local_stub& parent_) : parent(parent_) {}
      ~cu_cp_tx_pdu_notifier() { parent.disconnect(); }

      [[nodiscard]] bool on_new_message(const ngap_message& msg) override
      {
        parent.handle_tx_message(msg);
        return true;
      }

    private:
      ngap_gateway_local_stub& parent;
    };

    // Store Rx PDU notifier
    cu_cp_rx_notifier = std::move(cu_cp_rx_pdu_notifier);

    return std::make_unique<cu_cp_tx_pdu_notifier>(*this);
  }

private:
  void disconnect() { cu_cp_rx_notifier.reset(); }

  // Handle message sent by CU-CP.
  void handle_tx_message(const ngap_message& msg)
  {
    using namespace asn1::ngap;

    // Save message to pcap.
    if (pcap_writer.is_write_enabled()) {
      byte_buffer   packed_pdu;
      asn1::bit_ref bref{packed_pdu};
      if (msg.pdu.pack(bref) == asn1::OCUDUASN_SUCCESS) {
        pcap_writer.push_pdu(std::move(packed_pdu));
      } else {
        logger.warning("Failed to encode NGAP Tx PDU.");
      }
    }

    if (msg.pdu.type().value != ngap_pdu_c::types_opts::init_msg) {
      return;
    }

    const auto msg_type = msg.pdu.init_msg().value.type().value;

    if (msg_type == ngap_elem_procs_o::init_msg_c::types_opts::ng_setup_request) {
      // CU-CP is requesting an NG Setup. Automatically reply with NG Setup Response.

      const auto& req = msg.pdu.init_msg().value.ng_setup_request();
      ocudu_assert(req->supported_ta_list.size() > 0, "NG Setup Request has no supported TA list");
      const auto& broadcast_plmns = req->supported_ta_list[0].broadcast_plmn_list;

      // Save PLMN and slice info for use in subsequent InitialContextSetupRequest messages.
      if (broadcast_plmns.size() > 0) {
        saved_plmn       = broadcast_plmns[0].plmn_id;
        saved_slice_list = broadcast_plmns[0].tai_slice_support_list;
      }

      // Generate fake NG Setup Response.
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

      // Support for the same PLMNs and Slices as in NG Setup request.
      ng_resp->plmn_support_list.resize(broadcast_plmns.size());
      for (unsigned i = 0; i != broadcast_plmns.size(); ++i) {
        auto& out_plmn              = ng_resp->plmn_support_list[i];
        out_plmn.plmn_id            = broadcast_plmns[i].plmn_id;
        out_plmn.slice_support_list = broadcast_plmns[i].tai_slice_support_list;
      }

      // Send NG Setup Response back to CU-CP.
      send_rx_pdu_to_cu_cp(resp);

    } else if (msg_type == ngap_elem_procs_o::init_msg_c::types_opts::init_ue_msg) {
      // UE sends RRCSetupComplete (InitialUEMessage). Respond with InitialContextSetupRequest
      // to trigger SecurityModeCommand exchange and register the UE.

      const auto& req          = msg.pdu.init_msg().value.init_ue_msg();
      const auto  ran_ue_id    = req->ran_ue_ngap_id;
      const auto  amf_ue_id    = next_amf_ue_id++;
      ran_to_amf_map[ran_ue_id] = amf_ue_id;

      ngap_message ics;
      ics.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_INIT_CONTEXT_SETUP);
      auto& r               = ics.pdu.init_msg().value.init_context_setup_request();
      r->amf_ue_ngap_id     = amf_ue_id;
      r->ran_ue_ngap_id     = ran_ue_id;

      // GUAMI: reuse the PLMN saved from NG Setup, fixed AMF identity.
      r->guami.plmn_id = saved_plmn;
      r->guami.amf_region_id.from_number(0x2);
      r->guami.amf_set_id.from_number(0x40);
      r->guami.amf_pointer.from_number(0x0);

      // NAS RegistrationAccept embedded in the ICS request (forwarded to UE via DL RRC).
      r->nas_pdu_present = true;
      r->nas_pdu.from_string(
          "7e02c4f6c22f017e0042010177000bf202f8998000410000001054070002f8990000011500210201005e01b6");

      // NIA2+NIA1 integrity support — gNB selects NIA2; NEA0 cipher (null) selected by gNB preference.
      r->ue_security_cap.nr_encryption_algorithms.from_number(0);
      r->ue_security_cap.nr_integrity_protection_algorithms.from_number(49152);
      r->ue_security_cap.eutr_aencryption_algorithms.from_number(0);
      r->ue_security_cap.eutr_aintegrity_protection_algorithms.from_number(0);

      // Deterministic 256-bit key (all ones) — not used for ciphering with NEA0/NIA0.
      r->security_key.from_string(
          "1111111111111111111111111111111111111111111111111111111111111111"
          "1111111111111111111111111111111111111111111111111111111111111111"
          "1111111111111111111111111111111111111111111111111111111111111111"
          "1111111111111111111111111111111111111111111111111111111111111111");

      // Allowed NSSAI: mirror the first slice from NG Setup (typically SST=1).
      if (saved_slice_list.size() > 0) {
        allowed_nssai_item_s item{};
        item.s_nssai.sst = saved_slice_list[0].s_nssai.sst;
        item.s_nssai.sd_present = saved_slice_list[0].s_nssai.sd_present;
        item.s_nssai.sd         = saved_slice_list[0].s_nssai.sd;
        r->allowed_nssai.push_back(item);
      } else {
        allowed_nssai_item_s item{};
        item.s_nssai.sst.from_number(1);
        r->allowed_nssai.push_back(item);
      }

      // Provide pre-built UE radio capabilities to skip the UECapabilityEnquiry round-trip.
      r->ue_radio_cap_present = true;
      r->ue_radio_cap.from_string(
          "02c0856c18033a047465a025f800082a00241260e000307838031bff001300098a00d00000001c058d006007a071e439f0000240400e"
          "03000000001001be00000000000002074000000000005038a12007000f00000004008010");

      send_rx_pdu_to_cu_cp(ics);

    } else if (msg_type == ngap_elem_procs_o::init_msg_c::types_opts::ue_context_release_request) {
      // Absorb silently — no_core mode keeps UEs connected indefinitely.
      // Responding with UEContextReleaseCommand would tear down the UE, defeating the
      // purpose of a long-running simulation.  The CU-CP will retry after its own timer.
    }
  }

  // Forward NGAP message to CU-CP.
  void send_rx_pdu_to_cu_cp(const ngap_message& msg)
  {
    ocudu_assert(cu_cp_rx_notifier != nullptr, "Adapter is disconnected");

    if (pcap_writer.is_write_enabled()) {
      // PCAP writer is enabled. Encode ASN.1 message and send to PCAP.
      byte_buffer         bytes;
      asn1::bit_ref       bref{bytes};
      asn1::OCUDUASN_CODE code = msg.pdu.pack(bref);
      if (code != asn1::OCUDUASN_SUCCESS) {
        logger.warning("Failed to encode NGAP Rx PDU. NGAP PCAP will miss some messages.");
      } else {
        pcap_writer.push_pdu(std::move(bytes));
      }
    }

    // Push message to CU-CP.
    cu_cp_rx_notifier->on_new_message(msg);
  }

  dlt_pcap&               pcap_writer;
  ocudulog::basic_logger& logger = ocudulog::fetch_basic_logger("CU-CP");

  std::unique_ptr<ngap_rx_message_notifier> cu_cp_rx_notifier;

  uint64_t                             next_amf_ue_id = 1;
  std::map<uint64_t, uint64_t>         ran_to_amf_map;
  asn1::fixed_octstring<3, true>       saved_plmn;
  asn1::ngap::slice_support_list_l     saved_slice_list;
};

/// \brief NGAP bridge that uses the IO broker to handle the SCTP connection
class n2_sctp_gateway_client : public n2_connection_client
{
public:
  n2_sctp_gateway_client(io_broker&                           broker_,
                         task_executor&                       io_rx_executor_,
                         const sctp_network_connector_config& sctp_,
                         dlt_pcap&                            pcap_) :
    broker(broker_), sctp_cfg(sctp_), pcap_writer(pcap_)
  {
    // Create SCTP network adapter.
    sctp_gateway = create_sctp_network_client(sctp_network_client_config{sctp_cfg, broker, io_rx_executor_});
    report_error_if_not(sctp_gateway != nullptr, "Failed to create SCTP gateway client.\n");
  }

  std::unique_ptr<ngap_message_notifier>
  handle_cu_cp_connection_request(std::unique_ptr<ngap_rx_message_notifier> cu_cp_rx_pdu_notifier) override
  {
    ocudu_assert(cu_cp_rx_pdu_notifier != nullptr, "CU-CP Rx PDU notifier is null");

    // Establish SCTP connection and register SCTP Rx message handler.
    logger.debug("Establishing TNL connection to {} ({}:{})...",
                 sctp_cfg.dest_name,
                 sctp_cfg.connect_addresses[0],
                 sctp_cfg.connect_port);
    std::unique_ptr<sctp_association_sdu_notifier> sctp_sender = sctp_gateway->connect(
        std::make_unique<sctp_to_n2_pdu_notifier>(std::move(cu_cp_rx_pdu_notifier), pcap_writer, logger));

    if (sctp_sender == nullptr) {
      logger.error("Failed to establish N2 TNL connection to AMF on {}:{}.",
                   sctp_cfg.connect_addresses[0],
                   sctp_cfg.connect_port);
      return nullptr;
    }
    logger.info("{}: Connection to {} on {}:{} was established",
                sctp_cfg.if_name,
                sctp_cfg.dest_name,
                sctp_cfg.connect_addresses[0],
                sctp_cfg.connect_port);
    fmt::print("{}: Connection to {} on {}:{} completed\n",
               sctp_cfg.if_name,
               sctp_cfg.dest_name,
               sctp_cfg.connect_addresses[0],
               sctp_cfg.connect_port);

    // Return the Tx PDU notifier to the CU-CP.
    return std::make_unique<n2_to_sctp_pdu_notifier>(std::move(sctp_sender), pcap_writer, logger);
  }

private:
  io_broker&                          broker;
  const sctp_network_connector_config sctp_cfg;
  dlt_pcap&                           pcap_writer;
  ocudulog::basic_logger&             logger = ocudulog::fetch_basic_logger("CU-CP");

  // SCTP network adapter
  std::unique_ptr<sctp_network_client> sctp_gateway;
};

} // namespace

std::unique_ptr<n2_connection_client>
ocudu::ocucp::create_n2_connection_client(const n2_connection_client_config& params)
{
  if (std::holds_alternative<n2_connection_client_config::no_core>(params.mode)) {
    // Connection to local AMF stub.
    return std::make_unique<ngap_gateway_local_stub>(params.pcap);
  }

  // Connection to AMF through SCTP.
  const auto& nw_mode = std::get<n2_connection_client_config::network>(params.mode);
  return std::make_unique<n2_sctp_gateway_client>(nw_mode.broker, nw_mode.io_rx_executor, nw_mode.sctp, params.pcap);
}
