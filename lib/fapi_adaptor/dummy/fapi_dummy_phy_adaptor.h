// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "fapi_dummy_config.h"
#include "ocudu/fapi_adaptor/phy/phy_fapi_adaptor.h"
#include "ocudu/support/executors/task_executor.h"
#include <memory>
#include <vector>

namespace ocudu {
namespace fapi_adaptor {

class fapi_dummy_sector;
class fapi_dummy_timing_handler;

/// \brief Dummy PHY-FAPI adaptor.
///
/// Implements phy_fapi_adaptor at the FAPI boundary without any underlying PHY stack.
/// Drives slot timing from the system clock and simulates UE behaviour (RACH, UL ACKs).
///
/// The timing loop starts immediately on construction and stops on destruction.
class fapi_dummy_phy_adaptor : public phy_fapi_adaptor
{
public:
  fapi_dummy_phy_adaptor(const fapi_dummy_config& cfg, task_executor& executor);
  ~fapi_dummy_phy_adaptor() override;

  // See phy_fapi_adaptor for documentation.
  phy_fapi_sector_adaptor& get_sector_adaptor(unsigned cell_id) override;

private:
  fapi_dummy_config                               cfg;
  std::vector<std::unique_ptr<fapi_dummy_sector>> sectors;
  std::unique_ptr<fapi_dummy_timing_handler>      timing_handler;
};

} // namespace fapi_adaptor
} // namespace ocudu
