/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "procedures/f1ap_positioning_activation_procedure.h"
#include "procedures/f1ap_positioning_information_exchange_procedure.h"
#include "procedures/f1ap_positioning_measurement_procedure.h"
#include "procedures/f1ap_trp_information_exchange_procedure.h"
#include "ocudu/adt/expected.h"
#include "ocudu/ran/positioning/measurement_information.h"

using namespace ocudu;
using namespace asn1::f1ap;
using namespace ocucp;

async_task<expected<trp_information_response_t, trp_information_failure_t>>
f1ap_cu_impl::handle_trp_information_request(const trp_information_request_t& request)
{
  logger.info("Handling TRP information request");
  return launch_async<f1ap_trp_information_exchange_procedure>(cfg, request, ev_mng, tx_pdu_notifier, logger);
}

async_task<expected<positioning_information_response_t, positioning_information_failure_t>>
f1ap_cu_impl::handle_positioning_information_request(const positioning_information_request_t& request)
{
  logger.info("Handling positioning information request");

  if (!ue_ctxt_list.contains(request.ue_index)) {
    logger.warning("ue={}: Dropping \"UEPositioningInformationRequest\". UE context does not exist", request.ue_index);

    return launch_async(
        [](coro_context<async_task<expected<positioning_information_response_t, positioning_information_failure_t>>>&
               ctx) mutable {
          CORO_BEGIN(ctx);
          CORO_RETURN(make_unexpected(positioning_information_failure_t{}));
        });
  }

  return launch_async<f1ap_positioning_information_exchange_procedure>(
      cfg, request, ue_ctxt_list[request.ue_index], tx_pdu_notifier, logger);
}

async_task<expected<positioning_activation_response_t, positioning_activation_failure_t>>
f1ap_cu_impl::handle_positioning_activation_request(const positioning_activation_request_t& request)
{
  if (!ue_ctxt_list.contains(request.ue_index)) {
    logger.warning("ue={}: Dropping \"UEPositioningActivationRequest\". UE context does not exist", request.ue_index);

    return launch_async(
        [](coro_context<async_task<expected<positioning_activation_response_t, positioning_activation_failure_t>>>&
               ctx) mutable {
          CORO_BEGIN(ctx);
          CORO_RETURN(make_unexpected(positioning_activation_failure_t{}));
        });
  }

  f1ap_ue_context& ue_ctxt = ue_ctxt_list[request.ue_index];
  ue_ctxt.logger.log_info("Handling positioning activation request");

  return launch_async<f1ap_positioning_activation_procedure>(cfg, request, ue_ctxt, tx_pdu_notifier, logger);
}

async_task<expected<measurement_response_t, measurement_failure_t>>
f1ap_cu_impl::handle_positioning_measurement_request(const measurement_request_t& request)
{
  logger.info("Handling positioning measurement request");
  return launch_async<f1ap_positioning_measurement_procedure>(cfg, request, ev_mng, tx_pdu_notifier, logger);
}
