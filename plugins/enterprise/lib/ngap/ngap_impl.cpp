/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ngap_impl.h"

using namespace ocudu;
using namespace asn1::ngap;
using namespace ocucp;

void ngap_impl::handle_dl_ue_associated_nrppa_transport(const asn1::ngap::dl_ue_associated_nrppa_transport_s& msg)
{
  // Store routing id.
  context.lmf_routing_id = msg->routing_id.copy();

  if (!ue_ctxt_list.contains(uint_to_ran_ue_id(msg->ran_ue_ngap_id))) {
    logger.warning("ran_ue={} amf_ue={}: Dropping DlUeAssociatedNrppaTransport. UE context does not exist",
                   msg->ran_ue_ngap_id,
                   msg->amf_ue_ngap_id);
    send_error_indication(tx_pdu_notifier, logger, {}, {}, ngap_cause_radio_network_t::unknown_local_ue_ngap_id);
    return;
  }

  ngap_ue_context& ue_ctxt = ue_ctxt_list[uint_to_ran_ue_id(msg->ran_ue_ngap_id)];

  if (ue_ctxt.release_scheduled) {
    ue_ctxt.logger.log_info("Dropping DlUeAssociatedNrppaTransport. UE is already scheduled for release");
    stored_error_indications.emplace(ue_ctxt.ue_ids.ue_index,
                                     error_indication_request_t{ngap_cause_radio_network_t::interaction_with_other_proc,
                                                                ue_ctxt.ue_ids.ran_ue_id,
                                                                uint_to_amf_ue_id(msg->amf_ue_ngap_id)});
    return;
  }

  // Forward to CU-CP.
  cu_cp_notifier.on_dl_ue_associated_nrppa_transport_pdu(ue_ctxt.ue_ids.ue_index, msg->nrppa_pdu);
}

void ngap_impl::handle_dl_non_ue_associated_nrppa_transport(
    const asn1::ngap::dl_non_ue_associated_nrppa_transport_s& msg)
{
  // Store routing id.
  context.lmf_routing_id = msg->routing_id.copy();

  // Forward to CU-CP.
  cu_cp_notifier.on_dl_non_ue_associated_nrppa_transport_pdu(context.amf_index, msg->nrppa_pdu);
}

void ngap_impl::handle_ul_ue_associated_nrppa_transport(ue_index_t ue_index, const byte_buffer& nrppa_pdu)
{
  // Forward message to AMF.
  if (!ue_ctxt_list.contains(ue_index)) {
    logger.warning("ue={}: Dropping ULUEAssociatedNRPPATransport. UE context does not exist", ue_index);
    return;
  }

  ngap_ue_context& ue_ctxt = ue_ctxt_list[ue_index];

  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg();
  ngap_msg.pdu.init_msg().load_info_obj(ASN1_NGAP_ID_UL_UE_ASSOCIATED_NRPPA_TRANSPORT);
  ngap_msg.pdu.init_msg().value.ul_ue_associated_nrppa_transport()->routing_id = context.lmf_routing_id.copy();
  ngap_msg.pdu.init_msg().value.ul_ue_associated_nrppa_transport()->nrppa_pdu  = nrppa_pdu.copy();
  ngap_msg.pdu.init_msg().value.ul_ue_associated_nrppa_transport()->amf_ue_ngap_id =
      amf_ue_id_to_uint(ue_ctxt.ue_ids.amf_ue_id);
  ngap_msg.pdu.init_msg().value.ul_ue_associated_nrppa_transport()->ran_ue_ngap_id =
      ran_ue_id_to_uint(ue_ctxt.ue_ids.ran_ue_id);

  auto* ue = ue_ctxt.get_cu_cp_ue();
  ocudu_assert(ue != nullptr,
               "ue={} ran_ue={} amf_ue={}: UE for UE context doesn't exist",
               fmt::underlying(ue_ctxt.ue_ids.ue_index),
               fmt::underlying(ue_ctxt.ue_ids.ran_ue_id),
               fmt::underlying(ue_ctxt.ue_ids.amf_ue_id));

  // Schedule transmission of UE associated NRPPA transport.
  ue->schedule_async_task(launch_async([this, ue_index, ngap_msg](coro_context<async_task<void>>& ctx) {
    CORO_BEGIN(ctx);

    if (!ue_ctxt_list.contains(ue_index)) {
      logger.warning("ue={} ran_ue={} amf_ue={}: Dropping scheduled ULUEAssociatedNRPPATransport. UE context does not "
                     "exist anymore",
                     ue_index,
                     ngap_msg.pdu.init_msg().value.ul_ue_associated_nrppa_transport()->ran_ue_ngap_id,
                     ngap_msg.pdu.init_msg().value.ul_ue_associated_nrppa_transport()->amf_ue_ngap_id);
    } else {
      if (!tx_pdu_notifier.on_new_message(ngap_msg)) {
        logger.error("ue={} ran_ue={} amf_ue={}: AMF notifier is not set. Cannot send ULUEAssociatedNRPPATransport",
                     ue_index,
                     ngap_msg.pdu.init_msg().value.ul_ue_associated_nrppa_transport()->ran_ue_ngap_id,
                     ngap_msg.pdu.init_msg().value.ul_ue_associated_nrppa_transport()->amf_ue_ngap_id);
        CORO_EARLY_RETURN();
      }
    }
    CORO_RETURN();
  }));
}

async_task<void> ngap_impl::handle_ul_non_ue_associated_nrppa_transport(const byte_buffer& nrppa_pdu)
{
  // Forward message to AMF.
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg();
  ngap_msg.pdu.init_msg().load_info_obj(ASN1_NGAP_ID_UL_NON_UE_ASSOCIATED_NRPPA_TRANSPORT);
  ngap_msg.pdu.init_msg().value.ul_non_ue_associated_nrppa_transport()->routing_id = context.lmf_routing_id.copy();
  ngap_msg.pdu.init_msg().value.ul_non_ue_associated_nrppa_transport()->nrppa_pdu  = nrppa_pdu.copy();

  return launch_async([this, ngap_msg](coro_context<async_task<void>>& ctx) {
    CORO_BEGIN(ctx);

    // Transmit non UE associated NRPPA transport.
    if (!tx_pdu_notifier.on_new_message(ngap_msg)) {
      logger.warning("AMF notifier is not set. Cannot send ULnonUEAssociatedNRPPATransport");
      CORO_EARLY_RETURN();
    }
    CORO_RETURN();
  });
}
