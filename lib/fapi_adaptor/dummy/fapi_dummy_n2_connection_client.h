// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/ngap/gateways/n2_connection_client.h"
#include <memory>

namespace ocudu {

class dlt_pcap;

namespace fapi_adaptor {

/// \brief Creates a no_core AMF stub for use with the fapi_dummy PHY plugin.
///
/// Extends the standard no_core stub by including a PDU session in the
/// InitialContextSetupRequest, which causes the CU-CP to set up DRBs after
/// SecurityModeComplete and exercise the full L2/L3 data path.
std::unique_ptr<ocucp::n2_connection_client> create_fapi_dummy_n2_connection_client(dlt_pcap& pcap);

} // namespace fapi_adaptor
} // namespace ocudu
