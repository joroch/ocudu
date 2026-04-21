// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/ran/subcarrier_spacing.h"
#include <vector>

namespace ocudu {
namespace fapi_adaptor {

/// Configuration for simulated UEs within a dummy FAPI cell.
struct fapi_dummy_ue_config {
  /// Number of UEs to simulate in this cell. 0 means slot timing only, no UEs.
  unsigned nof_ues = 0;
  /// Number of slots between successive RACH indications for each simulated UE.
  unsigned ue_creation_stagger_slots = 10;
  /// Additional slot offset before the first RACH in this cell. Used to stagger
  /// multi-cell attach sequences so that DRB setup events across cells do not
  /// land in the same scheduler slot and trigger a data race in the shared
  /// logical_channel_system::lc_mapper.
  unsigned rach_start_offset_slots = 0;
};

/// Per-cell configuration for the dummy FAPI PHY adaptor.
struct fapi_dummy_cell_config {
  /// Subcarrier spacing, determines the slot duration (e.g. 30 kHz → 0.5 ms/slot).
  subcarrier_spacing scs = subcarrier_spacing::kHz15;
  /// UE simulation configuration for this cell.
  fapi_dummy_ue_config ue;
};

/// Configuration for the dummy FAPI PHY adaptor.
struct fapi_dummy_config {
  /// When false the factory returns the no-op adaptor unchanged.
  bool enabled = false;
  /// UE configuration applied to all cells (overridden per-cell in fapi_dummy_cell_config).
  fapi_dummy_ue_config ue;
  /// Per-cell configuration. Populated by the plugin from fapi_cfg + parsed YAML.
  std::vector<fapi_dummy_cell_config> cells;
};

} // namespace fapi_adaptor
} // namespace ocudu
