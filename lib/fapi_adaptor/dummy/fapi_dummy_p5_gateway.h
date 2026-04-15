// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/fapi/p5/p5_requests_gateway.h"
#include <functional>

namespace ocudu {

namespace fapi {
class p5_responses_notifier;
} // namespace fapi

namespace fapi_adaptor {

/// \brief Dummy FAPI P5 requests gateway.
///
/// Handles all FAPI P5 control messages (PARAM/CONFIG/START/STOP) and immediately
/// generates the corresponding success responses back to the MAC via p5_responses_notifier.
///
/// start_request and stop_request additionally invoke caller-supplied callbacks so that
/// higher-level components (e.g. the timing handler) can react to cell lifecycle events.
class fapi_dummy_p5_gateway : public fapi::p5_requests_gateway
{
public:
  fapi_dummy_p5_gateway() = default;

  /// Sets the P5 responses notifier used to send responses back to the MAC.
  void set_notifier(fapi::p5_responses_notifier& n) { notifier = &n; }

  /// Sets the callback invoked when a start_request is received.
  void set_start_callback(std::function<void()> cb) { on_start_cb = std::move(cb); }

  /// Sets the callback invoked when a stop_request is received.
  void set_stop_callback(std::function<void()> cb) { on_stop_cb = std::move(cb); }

  // See fapi::p5_requests_gateway for documentation.
  void send_param_request(const fapi::param_request& msg) override;

  // See fapi::p5_requests_gateway for documentation.
  void send_config_request(const fapi::config_request& msg) override;

  // See fapi::p5_requests_gateway for documentation.
  void send_start_request(const fapi::start_request& msg) override;

  // See fapi::p5_requests_gateway for documentation.
  void send_stop_request(const fapi::stop_request& msg) override;

private:
  fapi::p5_responses_notifier* notifier    = nullptr;
  std::function<void()>        on_start_cb;
  std::function<void()>        on_stop_cb;
};

} // namespace fapi_adaptor
} // namespace ocudu
