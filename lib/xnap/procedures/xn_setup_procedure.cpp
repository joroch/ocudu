/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "xn_setup_procedure.h"
#include "ocudu/asn1/xnap/common.h"
#include "ocudu/asn1/xnap/xnap_pdu_contents.h"
#include "ocudu/xnap/gateways/xnc_connection_gateway.h"

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace asn1::xnap;

xn_setup_procedure::xn_setup_procedure(xnap_configuration      xnap_cfg_,
                                       xnc_connection_gateway& xnc_gw_,
                                       timer_factory           timers_,
                                       ocudulog::basic_logger& logger_) :
  xnap_cfg(xnap_cfg_), xnc_gw(xnc_gw_), logger(logger_)
{
}

void xn_setup_procedure::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);
  fmt::println("XN SETUP PROCEDURE");
  logger.debug("\"{}\" started...", name());

  // Prepare XN Setup message
  fill_xn_setup_message();

  // Send XN Setup message;
  send_xn_setup_message();
  CORO_RETURN();
}

void xn_setup_procedure::fill_xn_setup_message()
{
  xn_setup_req.pdu.set_init_msg();
  xn_setup_req.pdu.init_msg().load_info_obj(ASN1_XNAP_ID_XN_SETUP);

  xn_setup_request_s& asn1_ies = xn_setup_req.pdu.init_msg().value.xn_setup_request();

  // Fill global RAN node id.
  auto& global_gnb = asn1_ies->global_ng_ran_node_id.set_gnb();
  global_gnb.gnb_id.set_gnb_id();
  global_gnb.gnb_id.gnb_id().from_number(1, 22); // TODO get right values.

  // Fill TAI support list.
  // TODO for loop.
  asn1_ies->tai_support_list.resize(1);
  for (auto& asn1_tai_support_item : asn1_ies->tai_support_list) {
    // Fill TAC.
    asn1_tai_support_item.tac.from_number(7);

    // Fill broadcast PLMN list.
    // TODO for loop
    asn1_tai_support_item.broadcast_plmns.resize(1);
    for (asn1::xnap::broadcast_plmn_in_tai_support_item_s& asn1_broadcast_plmn_item :
         asn1_tai_support_item.broadcast_plmns) {
      // Fill PLMN id.
      asn1_broadcast_plmn_item.plmn_id; // TODO.

      // Fill TAI slice support list.
      asn1_broadcast_plmn_item.tai_slice_support_list.resize(1);
      for (const asn1::xnap::s_nssai_s& ans1_slice_support_item : asn1_broadcast_plmn_item.tai_slice_support_list) {
        // TODO Fill S-NSSAI.
      }
    }
  }

  // Fill AMF region information.
  asn1_ies->amf_region_info.resize(1);
}

void xn_setup_procedure::send_xn_setup_message()
{
  // pack XNAP PDU into SCTP SDU.
  byte_buffer   tx_sdu{byte_buffer::fallback_allocation_tag{}};
  asn1::bit_ref bref(tx_sdu);
  if (xn_setup_req.pdu.pack(bref) != asn1::OCUDUASN_SUCCESS) {
    logger.error("Failed to pack F1AP PDU");
    return;
  }

  auto dest_addr = transport_layer_address::create_from_string(xnap_cfg.peer_addr);
  dest_addr.set_port(38422);
  logger.error("Sending XNAP PDU. bytes:{}", tx_sdu.length());
  xnc_gw.init_association(dest_addr, std::move(tx_sdu));
  logger.error("XNAP PDU sent.");
}
