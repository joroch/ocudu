/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "f1ap_test_message_validators.h"
#include "ocudu/f1ap/f1ap_message.h"

using namespace ocudu;
using namespace asn1::f1ap;

#define TRUE_OR_RETURN(cond)                                                                                           \
  if (not(cond))                                                                                                       \
    return false;

static bool is_packable(const f1ap_message& msg)
{
  byte_buffer   temp_pdu;
  asn1::bit_ref bref{temp_pdu};
  return msg.pdu.pack(bref) == asn1::OCUDUASN_SUCCESS;
}

bool test_helpers::is_valid_positioning_information_response(const f1ap_message& msg)
{
  TRUE_OR_RETURN(msg.pdu.type().value == f1ap_pdu_c::types_opts::successful_outcome);
  TRUE_OR_RETURN(msg.pdu.successful_outcome().value.type().value ==
                 f1ap_elem_procs_o::successful_outcome_c::types_opts::positioning_info_resp);
  TRUE_OR_RETURN(is_packable(msg));
  return true;
}

bool test_helpers::is_valid_f1ap_trp_information_request(const f1ap_message& msg)
{
  TRUE_OR_RETURN(msg.pdu.type().value == f1ap_pdu_c::types_opts::init_msg);
  TRUE_OR_RETURN(msg.pdu.init_msg().value.type().value == f1ap_elem_procs_o::init_msg_c::types_opts::trp_info_request);
  TRUE_OR_RETURN(is_packable(msg));
  return true;
}

bool test_helpers::is_valid_f1ap_trp_information_response(const f1ap_message& msg)
{
  TRUE_OR_RETURN(msg.pdu.type().value == f1ap_pdu_c::types_opts::successful_outcome);
  TRUE_OR_RETURN(msg.pdu.successful_outcome().value.type().value ==
                 f1ap_elem_procs_o::successful_outcome_c::types_opts::trp_info_resp);
  TRUE_OR_RETURN(is_packable(msg));
  return true;
}

bool test_helpers::is_valid_f1ap_positioning_information_request(const f1ap_message& msg)
{
  TRUE_OR_RETURN(msg.pdu.type().value == f1ap_pdu_c::types_opts::init_msg);
  TRUE_OR_RETURN(msg.pdu.init_msg().value.type().value ==
                 f1ap_elem_procs_o::init_msg_c::types_opts::positioning_info_request);
  TRUE_OR_RETURN(is_packable(msg));
  return true;
}

bool test_helpers::is_valid_f1ap_positioning_information_response(const f1ap_message& msg)
{
  TRUE_OR_RETURN(msg.pdu.type().value == f1ap_pdu_c::types_opts::successful_outcome);
  TRUE_OR_RETURN(msg.pdu.successful_outcome().value.type().value ==
                 f1ap_elem_procs_o::successful_outcome_c::types_opts::positioning_info_resp);
  TRUE_OR_RETURN(is_packable(msg));
  return true;
}

bool test_helpers::is_valid_f1ap_positioning_activation_request(const f1ap_message& msg)
{
  TRUE_OR_RETURN(msg.pdu.type().value == f1ap_pdu_c::types_opts::init_msg);
  TRUE_OR_RETURN(msg.pdu.init_msg().value.type().value ==
                 f1ap_elem_procs_o::init_msg_c::types_opts::positioning_activation_request);
  TRUE_OR_RETURN(is_packable(msg));
  return true;
}

bool test_helpers::is_valid_f1ap_positioning_measurement_request(const f1ap_message& msg)
{
  TRUE_OR_RETURN(msg.pdu.type().value == f1ap_pdu_c::types_opts::init_msg);
  TRUE_OR_RETURN(msg.pdu.init_msg().value.type().value ==
                 f1ap_elem_procs_o::init_msg_c::types_opts::positioning_meas_request);
  TRUE_OR_RETURN(is_packable(msg));
  return true;
}

bool test_helpers::is_valid_f1ap_positioning_measurement_response(const f1ap_message& msg)
{
  TRUE_OR_RETURN(msg.pdu.type().value == f1ap_pdu_c::types_opts::successful_outcome);
  TRUE_OR_RETURN(msg.pdu.successful_outcome().value.type().value ==
                 f1ap_elem_procs_o::successful_outcome_c::types_opts::positioning_meas_resp);
  TRUE_OR_RETURN(is_packable(msg));
  return true;
}

bool test_helpers::is_valid_f1ap_positioning_measurement_failure(const f1ap_message& msg)
{
  TRUE_OR_RETURN(msg.pdu.type().value == f1ap_pdu_c::types_opts::unsuccessful_outcome);
  TRUE_OR_RETURN(msg.pdu.unsuccessful_outcome().value.type().value ==
                 f1ap_elem_procs_o::unsuccessful_outcome_c::types_opts::positioning_meas_fail);
  TRUE_OR_RETURN(is_packable(msg));
  return true;
}
