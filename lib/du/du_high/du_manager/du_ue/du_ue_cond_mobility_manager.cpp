/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_ue_cond_mobility_manager.h"
#include <algorithm>

using namespace ocudu;
using namespace ocudu::odu;

void du_ue_cond_mobility_manager::add_prepared_cell(const nr_cell_global_id_t& cgi)
{
  auto it = std::find(cells.begin(), cells.end(), cgi);
  if (it == cells.end()) {
    cells.push_back(cgi);
  }
}

void du_ue_cond_mobility_manager::replace_prepared_cell(const nr_cell_global_id_t& cgi)
{
  cells.clear();
  cells.push_back(cgi);
}

void du_ue_cond_mobility_manager::cancel_prepared_cells(span<const nr_cell_global_id_t> cells_to_cancel)
{
  if (cells_to_cancel.empty()) {
    cells.clear();
    return;
  }
  cells.erase(std::remove_if(cells.begin(),
                             cells.end(),
                             [&cells_to_cancel](const nr_cell_global_id_t& cgi) {
                               return std::find(cells_to_cancel.begin(), cells_to_cancel.end(), cgi) !=
                                      cells_to_cancel.end();
                             }),
              cells.end());
}
