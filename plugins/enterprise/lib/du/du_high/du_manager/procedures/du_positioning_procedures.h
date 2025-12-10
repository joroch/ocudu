/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "du_ue/du_ue_manager.h"
#include "ocudu/f1ap/du/f1ap_du_positioning_handler.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu::odu {

/// Initiate a DU UE positioning information collection procedure.
async_task<du_positioning_info_response> start_du_ue_positioning_info_procedure(const du_positioning_info_request& msg,
                                                                                du_cell_manager& du_cells_,
                                                                                du_ue_manager&   ue_mng_);

/// Initiate a DU UE positioning measurement procedure.
async_task<du_positioning_meas_response>
start_positioning_measurement_procedure(const du_positioning_meas_request&     req,
                                        du_cell_manager&                       du_cells,
                                        du_ue_manager&                         ue_mng,
                                        const du_manager_params&               du_params,
                                        const std::map<trp_id_t, du_trp_info>& trps);

} // namespace ocudu::odu
