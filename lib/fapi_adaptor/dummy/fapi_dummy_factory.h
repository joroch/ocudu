// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "fapi_dummy_config.h"
#include "ocudu/fapi_adaptor/phy/phy_fapi_adaptor.h"
#include <memory>

namespace ocudu {

class task_executor;

namespace fapi_adaptor {

/// \brief Creates a dummy PHY-FAPI adaptor.
///
/// The returned adaptor drives slot timing from the system clock, simulates RACH-based UE
/// attach for each configured cell, and ACKs all uplink grants sent by the MAC.
///
/// \param cfg      Dummy PHY configuration (cells, nof_ues, stagger, etc.).
/// \param executor Task executor used for the slot-timing loop.
/// \returns        A fully constructed dummy phy_fapi_adaptor.
std::unique_ptr<phy_fapi_adaptor> create_fapi_dummy_phy_adaptor(const fapi_dummy_config& cfg,
                                                                 task_executor&           executor);

} // namespace fapi_adaptor
} // namespace ocudu
