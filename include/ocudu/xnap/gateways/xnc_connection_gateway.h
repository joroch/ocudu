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

#include "ocudu/cu_cp/cu_cp_xnc_handler.h"
#include <cstdint>
#include <optional>

namespace ocudu {
namespace ocucp {

/// Connection gateway responsible for handling new connection requests/drops coming
/// from neighbour gNBs via the XN-C interface and converting them to CU-CP commands.
class xnc_connection_gateway
{
public:
  virtual ~xnc_connection_gateway() = default;

  /// Attach a XN-C handler to the XN-C gateway.
  virtual void attach_cu_cp(cu_cp_xnc_handler& xnc_handler_) = 0;

  /// Get port on which the F1-C Server is listening for new connections.
  ///
  /// This method is useful in testing, where we don't want to use a specific port.
  /// \return The port number on which the F1-C Server is listening for new connections.
  virtual std::optional<uint16_t> get_listen_port() const = 0;
};

} // namespace ocucp
} // namespace ocudu
