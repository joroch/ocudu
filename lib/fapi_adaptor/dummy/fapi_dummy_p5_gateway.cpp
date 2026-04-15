// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "fapi_dummy_p5_gateway.h"
#include "ocudu/fapi/common/error_code.h"
#include "ocudu/fapi/p5/p5_messages.h"
#include "ocudu/fapi/p5/p5_responses_notifier.h"

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

void fapi_dummy_p5_gateway::send_param_request(const fapi::param_request& msg)
{
  if (notifier) {
    notifier->on_param_response({fapi::error_code_id::msg_ok});
  }
}

void fapi_dummy_p5_gateway::send_config_request(const fapi::config_request& msg)
{
  if (notifier) {
    notifier->on_config_response({fapi::error_code_id::msg_ok});
  }
}

void fapi_dummy_p5_gateway::send_start_request(const fapi::start_request& msg)
{
  if (on_start_cb) {
    on_start_cb();
  }
}

void fapi_dummy_p5_gateway::send_stop_request(const fapi::stop_request& msg)
{
  if (on_stop_cb) {
    on_stop_cb();
  }
  if (notifier) {
    notifier->on_stop_indication({});
  }
}
