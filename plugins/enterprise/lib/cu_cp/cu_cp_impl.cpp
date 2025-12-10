/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cu_cp_impl.h"
#include "routines/positioning/trp_information_exchange_routine.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/nrppa/nrppa_factory.h"
#include "ocudu/support/async/coroutine.h"
#include <variant>

using namespace ocudu;
using namespace ocucp;

std::unique_ptr<nrppa_interface> cu_cp_impl::create_nrppa_entity(const cu_cp_configuration& cu_cp_cfg,
                                                                 nrppa_cu_cp_notifier&      cu_cp_notif,
                                                                 common_task_scheduler&     common_task_sched_)
{
  return create_nrppa(cu_cp_cfg, cu_cp_notif, common_task_sched_);
}

void cu_cp_impl::handle_dl_ue_associated_nrppa_transport_pdu(ue_index_t ue_index, const byte_buffer& nrppa_pdu)
{
  nrppa_entity->get_nrppa_message_handler().handle_new_nrppa_pdu(nrppa_pdu, ue_index);
}

void cu_cp_impl::handle_dl_non_ue_associated_nrppa_transport_pdu(amf_index_t amf_index, const byte_buffer& nrppa_pdu)
{
  nrppa_entity->get_nrppa_message_handler().handle_new_nrppa_pdu(nrppa_pdu, amf_index);
}

nrppa_cu_cp_ue_notifier* cu_cp_impl::handle_new_nrppa_ue(ue_index_t ue_index)
{
  auto* ue = ue_mng.find_ue(ue_index);
  if (ue == nullptr) {
    return nullptr;
  }
  return &ue->get_nrppa_cu_cp_ue_notifier();
}

void cu_cp_impl::handle_ul_nrppa_pdu(const byte_buffer&                    nrppa_pdu,
                                     std::variant<ue_index_t, amf_index_t> ue_or_amf_index)
{
  if (std::holds_alternative<ue_index_t>(ue_or_amf_index)) {
    ue_index_t ue_index = std::get<ue_index_t>(ue_or_amf_index);

    if (ue_mng.find_du_ue(ue_index) == nullptr) {
      logger.warning("UE index={} got removed", ue_index);
      return;
    }

    auto* ue = ue_mng.find_du_ue(ue_index);

    auto* ngap = ngap_db.find_ngap(ue->get_ue_context().plmn);
    if (ngap == nullptr) {
      logger.warning("NGAP not found for PLMN={}", ue->get_ue_context().plmn);
      return;
    }

    // Forward the NRPPa message to the NGAP.
    ngap->handle_ul_ue_associated_nrppa_transport(ue_index, nrppa_pdu);
  } else {
    amf_index_t amf_index = std::get<amf_index_t>(ue_or_amf_index);

    // Forward the NRPPa message to the NGAP.
    common_task_sched.schedule_async_task(
        launch_async([this, amf_index, nrppa_pdu](coro_context<async_task<void>>& ctx) {
          CORO_BEGIN(ctx);

          if (ngap_db.find_ngap(amf_index) == nullptr) {
            logger.warning("NGAP not found for AMF index={}", amf_index);
            CORO_EARLY_RETURN();
          }

          CORO_AWAIT(ngap_db.find_ngap(amf_index)->handle_ul_non_ue_associated_nrppa_transport(nrppa_pdu));
          CORO_RETURN();
        }));
  }
}

async_task<trp_information_cu_cp_response_t>
cu_cp_impl::handle_trp_information_request(const trp_information_request_t& request)
{
  return launch_async<trp_information_exchange_routine>(request, du_db, nrppa_f1ap_ev_notifiers);
}
