/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "f1ap/cu_cp/f1ap_cu_impl.h"
#include "ocudu/asn1/f1ap/f1ap.h"
#include "ocudu/f1ap/cu_cp/f1ap_configuration.h"
#include "ocudu/f1ap/cu_cp/f1ap_cu.h"
#include "ocudu/f1ap/f1ap_message_notifier.h"

namespace ocudu::ocucp {

/// \brief Positioning Measurement, TS 38.473 section 8.13.3.
/// The purpose of the Positioning Measurement procedure is to allow the gNB-CU to request one or more TRPs in the
/// gNB-DU to perform and report positioning measurements. The procedure uses non-UE-associated signalling.
class f1ap_positioning_measurement_procedure
{
public:
  f1ap_positioning_measurement_procedure(const f1ap_configuration&    f1ap_cfg,
                                         const measurement_request_t& request_,
                                         f1ap_event_manager&          ev_mng_,
                                         f1ap_message_notifier&       f1ap_notif_,
                                         ocudulog::basic_logger&      logger_);

  void operator()(coro_context<async_task<expected<measurement_response_t, measurement_failure_t>>>& ctx);

  static const char* name() { return "Positioning measurement Procedure"; }

private:
  /// Send F1 positioning measurement request to DU.
  void send_positioning_measurement_request();

  /// \brief Creates common type measurement failure.
  /// \param[in] cause The cause of the failure.
  /// \return The measurement failure PDU.
  measurement_failure_t create_positioning_measurement_failure(const f1ap_cause_t& cause);

  /// \brief Creates common type measurement response.
  /// \param[in] asn1_resp The ASN.1 measurement response.
  /// \return The measurement response PDU.
  measurement_response_t create_positioning_measurement_response(const asn1::f1ap::positioning_meas_resp_s& asn1_resp);

  /// \brief Fill the procedure result, log it and forward it to the CU-CP.
  expected<measurement_response_t, measurement_failure_t> handle_procedure_outcome();

  const f1ap_configuration    f1ap_cfg;
  const measurement_request_t request;
  f1ap_event_manager&         ev_mng;
  f1ap_message_notifier&      f1ap_notifier;
  ocudulog::basic_logger&     logger;

  expected<measurement_response_t, measurement_failure_t> procedure_outcome;

  f1ap_transaction transaction;
};

} // namespace ocudu::ocucp
