/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "xnap_message.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu::ocucp {

struct xnap_message;

// TODO.
struct xnap_configuration {};

/// This interface is used to push XNAP messages to the XNAP interface.
class xnap_message_handler
{
public:
  virtual ~xnap_message_handler() = default;

  /// Handle the incoming XNAP message.
  virtual void handle_message(const xnap_message& msg) = 0;
};

/// Handle NGAP interface management procedures as defined in TS 38.413 section 8.7.
class xnap_connection_manager
{
public:
  virtual ~xnap_connection_manager() = default;

  /// \brief Request a new TNL association to the AMF.
  virtual bool handle_xn_peer_tnl_connection_request() = 0;

  /// TODO Docs.
  virtual async_task<void> handle_xn_setup_request_required(unsigned max_setup_retries) = 0;
};

/// NGAP notifier to the CU-CP.
class xnap_cu_cp_notifier
{
public:
  virtual ~xnap_cu_cp_notifier() = default;

  // TODO.
};

/// Combined entry point for the NGAP object.
class xnap_interface : public xnap_message_handler, public xnap_connection_manager
{
public:
  virtual ~xnap_interface() = default;
};
} // namespace ocudu::ocucp
