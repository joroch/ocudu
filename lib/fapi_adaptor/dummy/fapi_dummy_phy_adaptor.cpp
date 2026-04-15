// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "fapi_dummy_phy_adaptor.h"
#include "fapi_dummy_sector.h"
#include "fapi_dummy_timing_handler.h"
#include "ocudu/support/error_handling.h"
#include "ocudu/support/ocudu_assert.h"

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

fapi_dummy_phy_adaptor::fapi_dummy_phy_adaptor(const fapi_dummy_config& cfg_, task_executor& executor) : cfg(cfg_)
{
  const unsigned nof_cells = cfg.cells.empty() ? 1U : static_cast<unsigned>(cfg.cells.size());
  sectors.reserve(nof_cells);

  for (unsigned i = 0; i != nof_cells; ++i) {
    const fapi_dummy_cell_config& cell_cfg = cfg.cells.empty() ? fapi_dummy_cell_config{} : cfg.cells[i];
    sectors.push_back(std::make_unique<fapi_dummy_sector>(cell_cfg));
  }

  // Collect raw sector pointers for the timing handler.
  std::vector<fapi_dummy_sector*> sector_ptrs;
  sector_ptrs.reserve(nof_cells);
  for (auto& s : sectors) {
    sector_ptrs.push_back(s.get());
  }

  // Derive SCS from the first cell (or default to 15 kHz).
  subcarrier_spacing scs = cfg.cells.empty() ? subcarrier_spacing::kHz15 : cfg.cells[0].scs;

  timing_handler = std::make_unique<fapi_dummy_timing_handler>(scs, executor, std::move(sector_ptrs));
  timing_handler->start();
}

fapi_dummy_phy_adaptor::~fapi_dummy_phy_adaptor()
{
  timing_handler->stop();
}

phy_fapi_sector_adaptor& fapi_dummy_phy_adaptor::get_sector_adaptor(unsigned cell_id)
{
  ocudu_assert(cell_id < sectors.size(), "Cell ID {} out of range (nof_cells={})", cell_id, sectors.size());
  return *sectors[cell_id];
}
