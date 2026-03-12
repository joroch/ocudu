// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "xn_setup_procedure.h"
#include "../xnap_asn1_utils.h"
#include "xn_setup_asn1_helpers.h"
#include "xn_setup_procedure_asn1_helpers.h"
#include "ocudu/support/async/async_timer.h"

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace asn1::xnap;

/// SCTP association timout. This large, as SCTP may need to retransmit INIT multiple times.
static constexpr std::chrono::milliseconds sctp_init_timeout{30000};

/// XN setup. This large, as SCTP may need to retransmit INIT multiple times.
static constexpr std::chrono::milliseconds xn_setup_response_timeout{5000};

xn_setup_procedure::xn_setup_procedure(
    const xnap_configuration&                                                                    xnap_cfg_,
    std::optional<xnap_context>&                                                                 peer_ctxt_,
    xnap_tx_pdu_notifier_with_logging&                                                           tx_notifier_,
    protocol_transaction_event_source<bool>&                                                     sctp_init_outcome_,
    protocol_transaction_event_source<asn1::xnap::xn_setup_resp_s, asn1::xnap::xn_setup_fail_s>& xn_setup_outcome_,
    timer_factory                                                                                timers_,
    ocudulog::basic_logger&                                                                      logger_) :
  xnap_cfg(xnap_cfg_),
  peer_ctxt(peer_ctxt_),
  tx_notifier(tx_notifier_),
  sctp_init_outcome(sctp_init_outcome_),
  xn_setup_outcome(xn_setup_outcome_),
  logger(logger_),
  xn_setup_wait_timer(timers_.create_timer())
{
}

void xn_setup_procedure::operator()(coro_context<async_task<bool>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.info("\"{}\" started...", name());

  // Prepare XN Setup message.
  xn_setup_req = generate_asn1_xn_setup_request(xnap_cfg);

  // Subscribe to respective publisher for SCTP  message.
  sctp_init_transaction_sink.subscribe_to(sctp_init_outcome, sctp_init_timeout);

  // Forward message to XN-C.
  if (!tx_notifier.on_xn_setup_request(xn_setup_req)) {
    logger.warning("Cannot send XNSetupRequest");
    CORO_EARLY_RETURN(false);
  }

  // Await SCTP association inicialization.
  CORO_AWAIT(sctp_init_transaction_sink);

  if (!sctp_init_transaction_sink.successful()) {
    logger.warning("\"{}\" failed. No successful outcome received", name());
    CORO_EARLY_RETURN(false);
  }
  if (!sctp_init_transaction_sink.response()) {
    logger.warning("\"{}\" failed. SCTP initializtaion failed", name());
    CORO_EARLY_RETURN(false);
  }

  // Subscribe to respective publisher to receive XN SETUP RESPONSE/FAILURE message.
  xn_setup_transaction_sink.subscribe_to(xn_setup_outcome, xn_setup_response_timeout);

  // Await XN Setup response.
  CORO_AWAIT(xn_setup_transaction_sink);

  while (retry_required()) {
    // Await timer.
    logger.debug("Reinitiating XN setup in {}s. Received XNSetupFailure with Time to Wait IE", time_to_wait.count());
    CORO_AWAIT(
        async_wait_for(xn_setup_wait_timer, std::chrono::duration_cast<std::chrono::milliseconds>(time_to_wait)));

    // Forward message to XN-C.
    if (!tx_notifier.on_xn_setup_request(xn_setup_req)) {
      logger.warning("Cannot send XNSetupRequest");
      CORO_EARLY_RETURN(false);
    }
    // Subscribe to respective publisher to receive XN SETUP RESPONSE/FAILURE message.
    xn_setup_transaction_sink.subscribe_to(xn_setup_outcome, xn_setup_response_timeout);

    // Await XN Setup response.
    CORO_AWAIT(xn_setup_transaction_sink);
  }

  if (!xn_setup_transaction_sink.successful()) {
    logger.warning("\"{}\" failed. No successful outcome received", name());
    CORO_EARLY_RETURN(false);
  }

  // Handle successful outcome. Validate received message and store peer context information.
  received_xn_setup_resp = xn_setup_transaction_sink.response();

  validation_error = validate_xn_setup_request_response(received_xn_setup_resp);
  if (not validation_error.has_value()) {
    logger.warning("\"{}\" failed.Cause: Received XN Setup Response is invalid", name());
    CORO_EARLY_RETURN(false);
  }

  // Store peer context information.
  peer_ctxt = create_peer_xnap_context(received_xn_setup_resp);

  logger.info("\"{}\" finished successfully", name());

  CORO_RETURN(true);
}

bool xn_setup_procedure::retry_required()
{
  if (xn_setup_transaction_sink.successful()) {
    // Success case.
    return false;
  }

  if (xn_setup_transaction_sink.timeout_expired()) {
    // Timeout case.
    logger.warning("\"{}\" timed out after {}ms", name(), xn_setup_response_timeout.count());
    return false;
  }

  if (!xn_setup_transaction_sink.failed()) {
    // No response received.
    logger.warning("\"{}\" failed. No response received", name());
    return false;
  }

  const asn1::xnap::xn_setup_fail_s& xn_fail = xn_setup_transaction_sink.failure();

  if (not xn_fail->time_to_wait_present) {
    // XN-C peer didn't command a waiting time.
    logger.warning("\"{}\": Stopping procedure. Cause: XN-C peer did not set any retry waiting time", name());
    logger.warning("\"{}\" failed. XN-C peer Cause: \"{}\"", name(), asn1_utils::get_cause_str(xn_fail->cause));
    return false;
  }

  time_to_wait = std::chrono::seconds{xn_fail->time_to_wait.to_number()};
  return true;
}
