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

#include "ocudu/adt/span.h"
#include "ocudu/ran/nr_cgi.h"
#include <vector>

namespace ocudu {
namespace odu {

/// \brief Tracks prepared conditional mobility target cells for a UE in the DU.
class du_ue_cond_mobility_manager
{
public:
  /// \brief Register a new CHO candidate cell (deduplicating).
  void add_prepared_cell(const nr_cell_global_id_t& cgi);

  /// \brief Replace all existing candidates with a single new cell.
  void replace_prepared_cell(const nr_cell_global_id_t& cgi);

  /// \brief Cancel prepared candidates. Cancels all if \p cells_to_cancel is empty.
  void cancel_prepared_cells(span<const nr_cell_global_id_t> cells_to_cancel);

  span<const nr_cell_global_id_t> prepared_cells() const { return cells; }

private:
  std::vector<nr_cell_global_id_t> cells;
};

} // namespace odu
} // namespace ocudu
