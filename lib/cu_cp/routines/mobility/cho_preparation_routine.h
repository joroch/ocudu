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

#include "../../cu_cp_impl_interface.h"
#include "../../ue_manager/ue_manager_impl.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace ocucp {

/// \brief Handles preparation of one CHO candidate through the intra-CU HO API.
///
/// This routine prepares a target cell for CHO by:
/// 1. Allocating target UE context at target DU
/// 2. Sending F1AP UE Context Setup to DU
/// 3. Building RRCReconfiguration with reconfigurationWithSync
/// 4. Returning CHO preparation data in cu_cp_intra_cu_handover_response::cho_preparation_result
///
/// Unlike normal handover, this does NOT send the RRC message to the UE.
/// The RRC message is stored and sent later during CHO execution.
class cho_candidate_preparation_routine
{
public:
  cho_candidate_preparation_routine(const cu_cp_intra_cu_handover_request& request_,
                                    du_index_t                             source_du_index_,
                                    const byte_buffer&                     target_cell_sib1_,
                                    f1ap_ue_context_manager&               source_du_f1ap_ue_ctxt_mng_,
                                    f1ap_ue_context_manager&               target_du_f1ap_ue_ctxt_mng_,
                                    cu_cp_impl_interface&                  cu_cp_handler_,
                                    ue_manager&                            ue_mng_,
                                    ocudulog::basic_logger&                logger_);

  void operator()(coro_context<async_task<cu_cp_intra_cu_handover_response>>& ctx);

  static const char* name() { return "CHO Candidate Preparation Routine"; }

private:
  const cu_cp_intra_cu_handover_request request;
  const du_index_t                      source_du_index;
  const byte_buffer                     target_cell_sib1;

  cu_cp_ue* source_ue = nullptr; // Pointer to UE in the source DU
  cu_cp_ue* target_ue = nullptr; // Pointer to UE in the target DU

  rrc_ue_transfer_context source_rrc_context;

  f1ap_ue_context_manager& source_du_f1ap_ue_ctxt_mng;
  f1ap_ue_context_manager& target_du_f1ap_ue_ctxt_mng;
  cu_cp_impl_interface&    cu_cp_handler;
  ue_manager&              ue_mng;
  up_config_update         next_config;
  ocudulog::basic_logger&  logger;

  // (sub-)routine requests
  f1ap_ue_context_setup_request            target_ue_context_setup_request;
  f1ap_ue_context_modification_request     source_ue_context_mod_request;
  rrc_reconfiguration_procedure_request    rrc_reconfig_args;
  rrc_ue_handover_reconfiguration_context  cho_cand_ctxt;
  e1ap_bearer_context_modification_request bearer_context_modification_request;

  // (sub-)routine results
  cu_cp_intra_cu_handover_response      response_msg;
  f1ap_ue_context_setup_response        target_ue_context_setup_response;
  f1ap_ue_context_modification_response source_ue_context_mod_response;

  bool                                  is_intra_du               = false;
  bool                                  cleanup_target_ue_on_fail = false;
  ue_index_t                            cleanup_target_ue_index   = ue_index_t::invalid;
  std::vector<f1ap_srb_to_setup>        empty_srbs_to_setup_list;
  const std::vector<f1ap_srb_to_setup>* srbs_to_setup_for_rrc     = nullptr;
  const f1ap_du_to_cu_rrc_info*         du_to_cu_rrc_info_for_rrc = nullptr;
};

} // namespace ocucp
} // namespace ocudu
