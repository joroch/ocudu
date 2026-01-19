/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "xnap_impl.h"
#include "procedures/xn_setup_procedure.h"
#include "ocudu/asn1/xnap/common.h"
#include "ocudu/xnap/xnap_message.h"

using namespace ocudu;
using namespace asn1::xnap;
using namespace ocucp;

xnap_impl::xnap_impl(const xnap_configuration& xnap_cfg_,
                     xnc_connection_gateway&   xnc_gw_,
                     xnap_cu_cp_notifier&      cu_cp_notifier_,
                     timer_manager&            timers_,
                     task_executor&            ctrl_exec_) :
  logger(ocudulog::fetch_basic_logger("XNAP")),
  xnap_cfg(xnap_cfg_),
  xnc_gw(xnc_gw_),
  cu_cp_notifier(cu_cp_notifier_),
  timers(timers_),
  ctrl_exec(ctrl_exec_),
  transaction_mng(timer_factory{timers_, ctrl_exec_}),
  tx_pdu_notifier(*this)
{
  fmt::println("XN-C peer: {}", xnap_cfg.peer_addr);
  peer_addr = transport_layer_address::create_from_string(xnap_cfg.peer_addr);
  peer_addr.set_port(38422);
}

// Note: For fwd declaration of member types, dtor cannot be trivial.
xnap_impl::~xnap_impl() {}

bool xnap_impl::handle_xn_peer_tnl_connection_request()
{
  // This could be a reconnection, so make sure the tx_pdu_notifier is released before creating a new one.
  if (tx_pdu_notifier.is_connected()) {
    tx_pdu_notifier.disconnect();
  }

  auto dest_addr = transport_layer_address::create_from_string(xnap_cfg.peer_addr);
  dest_addr.set_port(38422);
  //  xnc_gw.init_association(dest_addr);
  return true;
}

void xnap_impl::handle_message(const xnap_message& msg)
{
  // Run NGAP protocols in Control executor.
  if (not ctrl_exec.execute([this, msg]() {
        // log_rx_pdu(msg);

        switch (msg.pdu.type().value) {
          case xn_ap_pdu_c::types_opts::init_msg:
            handle_initiating_message(msg.pdu.init_msg());
            break;
          case xn_ap_pdu_c::types_opts::successful_outcome:
            handle_successful_outcome(msg.pdu.successful_outcome());
            break;
          case xn_ap_pdu_c::types_opts::unsuccessful_outcome:
            handle_unsuccessful_outcome(msg.pdu.unsuccessful_outcome());
            break;
          default:
            logger.error("Invalid PDU type");
            break;
        }
      })) {
    logger.error("Discarding Rx XNAP PDU. Cause: task queue is full");
  }
}

void xnap_impl::handle_initiating_message(const init_msg_s& msg)
{
  switch (msg.value.type().value) {
    case xnap_elem_procs_o::init_msg_c::types_opts::xn_setup_request:
      handle_xn_setup_request(msg.value.xn_setup_request());
      break;
    default:
      logger.error("Initiating message of type {} is not supported", msg.value.type().to_string());
  }
}

void xnap_impl::handle_successful_outcome(const successful_outcome_s& msg)
{
  // TODO.
}

void xnap_impl::handle_unsuccessful_outcome(const unsuccessful_outcome_s& msg)
{
  // TODO.
}

// TODO fix return type.
async_task<void> xnap_impl::handle_xn_setup_request_required(unsigned max_setup_retries)
{
  return launch_async<xn_setup_procedure>(xnap_cfg, xnc_gw, timer_factory{timers, ctrl_exec}, logger);
}

void xnap_impl::handle_xn_setup_request(const xn_setup_request_s& msg)
{
  fmt::println("Got XN setup");
  xnap_message xn_setup_resp;

  xn_setup_resp.pdu.set_successful_outcome();
  xn_setup_resp.pdu.successful_outcome().load_info_obj(ASN1_XNAP_ID_XN_SETUP);

  xn_setup_resp_s& asn1_ies = xn_setup_resp.pdu.successful_outcome().value.xn_setup_resp();

  // Fill global RAN node id.
  auto& global_gnb = asn1_ies->global_ng_ran_node_id.set_gnb();
  global_gnb.gnb_id.set_gnb_id();
  global_gnb.gnb_id.gnb_id().from_number(1, 22); // TODO get right values.

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

  tx_notifier->on_new_message(xn_setup_resp);
}

void xnap_impl::tx_pdu_notifier_with_logging::on_new_message(const xnap_message& msg)
{
  // TODO improve logging of TX messages.
  if (decorated == nullptr) {
    parent.logger.error("not passing message to TX");
    return; // TODO check return.
  }
  decorated->on_new_message(msg);
}
