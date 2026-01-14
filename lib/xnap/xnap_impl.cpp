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
  tx_pdu_notifier(*this)
{
  fmt::println("XN-C peer: {}", xnap_cfg.peer_addr);
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
  xnc_gw.init_association(dest_addr);
  return true;
}

void xnap_impl::handle_message(const xnap_message& msg) {}

// TODO fix return type.
async_task<void> xnap_impl::handle_xn_setup_request_required(unsigned max_setup_retries)
{
  xnap_message xnap_msg = {};
  xnap_msg.pdu.set_init_msg();
  xnap_msg.pdu.init_msg().load_info_obj(ASN1_XNAP_ID_XN_SETUP);

  auto& xn_setup_request = xnap_msg.pdu.init_msg().value.xn_setup_request();
  (void)xn_setup_request;
  // fill_asn1_ng_setup_request(ng_setup_request, context);

  /// return launch_async<ng_setup_procedure>(
  ///     context, ngap_msg, max_setup_retries, tx_pdu_notifier, ev_mng, timer_factory{timers, ctrl_exec}, logger);
  return {};
}

void xnap_impl::tx_pdu_notifier_with_logging::on_new_message(const xnap_message& msg)
{
  // TODO improve logging of TX messages.
  if (decorated == nullptr) {
    return; // TODO check return.
  }
  decorated->on_new_message(msg);
}
