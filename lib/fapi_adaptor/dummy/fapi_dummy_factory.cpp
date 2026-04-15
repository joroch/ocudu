// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "fapi_dummy_factory.h"
#include "fapi_dummy_phy_adaptor.h"

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

std::unique_ptr<phy_fapi_adaptor> fapi_adaptor::create_fapi_dummy_phy_adaptor(const fapi_dummy_config& cfg,
                                                                               task_executor&           executor)
{
  return std::make_unique<fapi_dummy_phy_adaptor>(cfg, executor);
}
