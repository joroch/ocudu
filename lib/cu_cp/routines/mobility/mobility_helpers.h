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

#include "../../up_resource_manager/up_resource_manager_impl.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/cu_cp/cu_cp_ue_messages.h"
#include "ocudu/e1ap/cu_cp/e1ap_cu_cp_bearer_context_update.h"
#include "ocudu/f1ap/cu_cp/f1ap_cu_ue_context_update.h"

namespace ocudu {
namespace ocucp {

class cu_cp_ue;

/// \brief Fills an F1AP UE Context Setup Request common to both handover and CHO preparation.
/// \param[out] setup_request       The request to populate.
/// \param[in]  cgi                 Target cell global identity.
/// \param[in]  srbs                SRBs to setup.
/// \param[in]  transfer_context    RRC UE transfer context from source UE.
/// \param[in]  next_config         UP config update derived from the source UE.
/// \param[in]  is_cho              Set to true to include conditional inter-DU mobility info (CHO preparation).
/// \return True on success, false otherwise.
bool fill_ue_context_setup_request(f1ap_ue_context_setup_request&               setup_request,
                                   nr_cell_global_id_t                          cgi,
                                   const static_vector<srb_id_t, MAX_NOF_SRBS>& srbs,
                                   const rrc_ue_transfer_context&               transfer_context,
                                   const up_config_update&                      next_config,
                                   bool                                         is_cho);

/// \brief Creates an SRB on the given UE via RRC with security enabled.
void create_srb(cu_cp_ue* ue, srb_id_t srb_id);

/// \brief Handle UE context setup response from target DU and prefills the Bearer context modification.
bool handle_context_setup_response(cu_cp_intra_cu_handover_response&         response_msg,
                                   e1ap_bearer_context_modification_request& bearer_context_modification_request,
                                   const f1ap_ue_context_setup_response&     target_ue_context_setup_response,
                                   up_config_update&                         next_config,
                                   const ocudulog::basic_logger&             logger,
                                   bool                                      reestablish_pdcp);

/// \brief Handler Bearer context modification response from CU-UP and prefill UE context modification for source DU.
bool handle_bearer_context_modification_response(
    cu_cp_intra_cu_handover_response&                response_msg,
    f1ap_ue_context_modification_request&            source_ue_context_mod_request,
    const e1ap_bearer_context_modification_response& bearer_context_modification_response,
    up_config_update&                                next_config,
    const ocudulog::basic_logger&                    logger);

} // namespace ocucp
} // namespace ocudu
