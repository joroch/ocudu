// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "fapi_dummy_sector.h"

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

fapi_dummy_sector::fapi_dummy_sector(const fapi_dummy_cell_config& cfg_) : cfg(cfg_)
{
  p7_gw.set_ue_simulator(ue_sim);
}

void fapi_dummy_sector::on_new_slot(slot_point_extended slot)
{
  p7_gw.on_new_slot(slot);
  ue_sim.on_new_slot(slot.without_hyper_sfn());
}
