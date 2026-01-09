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

#include "ocudu/gateways/sctp_network_gateway.h"
#include "ocudu/xnap/gateways/xnc_connection_gateway.h"

namespace ocudu {

class dlt_pcap;
class io_broker;
class task_executor;

/// Configuration of an SCTP-based F1-C Gateway.
struct xnc_sctp_gateway_config {
  /// SCTP configuration.
  sctp_network_gateway_config sctp;
  /// SCTP peer config.
  std::vector<sctp_network_peer_config> peers;
  /// IO broker responsible for handling SCTP Rx data and notifications.
  io_broker& broker;
  /// Execution context used to process received SCTP packets.
  task_executor& io_rx_executor;
  /// PCAP writer for the F1AP messages.
  dlt_pcap& pcap;
};

/// Creates an F1-C Gateway server that listens for incoming SCTP connections, packs/unpacks F1AP PDUs and forwards
/// them to the GW/CU-CP F1AP handler.
std::unique_ptr<ocucp::xnc_connection_gateway> create_xnc_gateway(const xnc_sctp_gateway_config& params);

} // namespace ocudu
