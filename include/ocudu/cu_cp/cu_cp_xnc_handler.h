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

#include "ocudu/gateways/sctp_network_server.h" // TODO THIS should not be included!!!
#include "ocudu/xnap/xnap_message_notifier.h"
#include <memory>

namespace ocudu {
namespace ocucp {

/// \brief Handler of the XN-C interface of the CU-CP.
///
/// This interface is used to forward XN-C peer connection updates to the CU-CP.
class cu_cp_xnc_handler
{
public:
  virtual ~cu_cp_xnc_handler() = default;

  /// \brief Handles the establishment of a new DU-to-CU-CP TNL association.
  ///
  /// \param xnap_tx_pdu_notifier Notifier that the CU-CP will use to push XNAP Tx PDUs to the F1-C GW. Once this
  /// notifier instance goes out of scope, the F1-C GW will be notified that the CU-CP wants to shutdown the connection.
  /// \return Notifier that the F1-C GW will use to forward F1AP PDUs to the CU-CP. If the caller lets the returned
  /// object go out of scope, the CU-CP will be notified that a GW event occurred that resulted in the association
  /// being shutdown.
  virtual std::unique_ptr<xnap_message_notifier>
  handle_new_xnc_connection(std::unique_ptr<xnap_message_notifier> xnap_tx_pdu_notifier,
                            const sctp_association_info&           assoc_info) = 0;
};

} // namespace ocucp
} // namespace ocudu
