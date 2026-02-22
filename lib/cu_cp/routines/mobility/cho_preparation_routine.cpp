/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cho_preparation_routine.h"
#include "../pdu_session_routine_helpers.h"
#include "mobility_helpers.h"
#include "ocudu/cu_cp/cu_cp_types.h"

using namespace ocudu;
using namespace ocudu::ocucp;

cho_candidate_preparation_routine::cho_candidate_preparation_routine(
    const cu_cp_intra_cu_handover_request& request_,
    du_index_t                             source_du_index_,
    const byte_buffer&                     target_cell_sib1_,
    f1ap_ue_context_manager&               source_du_f1ap_ue_ctxt_mng_,
    f1ap_ue_context_manager&               target_du_f1ap_ue_ctxt_mng_,
    cu_cp_impl_interface&                  cu_cp_handler_,
    ue_manager&                            ue_mng_,
    ocudulog::basic_logger&                logger_) :
  request(request_),
  source_du_index(source_du_index_),
  target_cell_sib1(target_cell_sib1_),
  source_du_f1ap_ue_ctxt_mng(source_du_f1ap_ue_ctxt_mng_),
  target_du_f1ap_ue_ctxt_mng(target_du_f1ap_ue_ctxt_mng_),
  cu_cp_handler(cu_cp_handler_),
  ue_mng(ue_mng_),
  logger(logger_)
{
}

void cho_candidate_preparation_routine::operator()(coro_context<async_task<cu_cp_intra_cu_handover_response>>& ctx)
{
  CORO_BEGIN(ctx);

  // Validate input.
  if (request.source_ue_index == ue_index_t::invalid) {
    logger.warning("\"{}\" - Invalid source UE index", name());
    CORO_EARLY_RETURN(response_msg);
  }

  if (request.target_pci == INVALID_PCI) {
    logger.warning("\"{}\" - Invalid target PCI", name());
    CORO_EARLY_RETURN(response_msg);
  }
  if (request.target_du_index == du_index_t::invalid) {
    logger.warning("\"{}\" - Invalid target DU index", name());
    CORO_EARLY_RETURN(response_msg);
  }
  if (!request.cho_preparation.has_value()) {
    logger.warning("\"{}\" - Missing CHO preparation information", name());
    CORO_EARLY_RETURN(response_msg);
  }

  // Retrieve source UE context.
  source_ue = ue_mng.find_du_ue(request.source_ue_index);
  if (source_ue == nullptr) {
    logger.warning("ue={}: \"{}\" - Source UE not found", request.source_ue_index, name());
    CORO_EARLY_RETURN(response_msg);
  }

  source_rrc_context = source_ue->get_rrc_ue()->get_transfer_context();
  next_config        = to_config_update(source_rrc_context.up_ctx);

  logger.debug("ue={}: \"{}\" started for target_pci={} cond_recfg_id={}...",
               request.source_ue_index,
               name(),
               request.target_pci,
               request.cho_preparation->cond_recfg_id);

  is_intra_du               = source_du_index == request.target_du_index;
  is_intra_du               = false; // For now always allocate new UE.
  cleanup_target_ue_on_fail = false;
  cleanup_target_ue_index   = ue_index_t::invalid;
  empty_srbs_to_setup_list.clear();
  srbs_to_setup_for_rrc     = &empty_srbs_to_setup_list;
  du_to_cu_rrc_info_for_rrc = nullptr;

  if (!is_intra_du) {
    // Inter-DU CHO: allocate target UE and prepare candidate via UE CONTEXT SETUP.
    target_ue_context_setup_request.ue_index = ue_mng.add_ue(request.target_du_index);
    if (target_ue_context_setup_request.ue_index == ue_index_t::invalid) {
      logger.warning("ue={}: \"{}\" failed to allocate UE index at target DU", request.source_ue_index, name());
      CORO_EARLY_RETURN(response_msg);
    }

    if (!cu_cp_handler.handle_ue_plmn_selected(target_ue_context_setup_request.ue_index,
                                               source_ue->get_ue_context().plmn)) {
      logger.warning("ue={}: \"{}\" failed to set PLMN for target UE", request.source_ue_index, name());
      ue_mng.remove_ue(target_ue_context_setup_request.ue_index);
      CORO_EARLY_RETURN(response_msg);
    }

    // Connect the target UE to the same CU-UP as the source UE.
    ue_mng.find_du_ue(target_ue_context_setup_request.ue_index)->set_cu_up_index(source_ue->get_cu_up_index());

    // Prepare F1AP UE Context Setup Command and call F1AP notifier of target DU.
    if (!fill_ue_context_setup_request(target_ue_context_setup_request,
                                       request.cgi,
                                       source_ue->get_rrc_ue()->get_srbs(),
                                       source_rrc_context,
                                       next_config,
                                       true)) {
      logger.warning("ue={}: \"{}\" failed to generate UeContextSetupRequest", request.source_ue_index, name());
      CORO_AWAIT(cu_cp_handler.handle_ue_removal_request(target_ue_context_setup_request.ue_index));
      CORO_EARLY_RETURN(response_msg);
    }

    target_ue_context_setup_request.cu_to_du_rrc_info.meas_cfg = source_ue->get_rrc_ue()->get_packed_meas_config();
    target_ue_context_setup_request.serving_cell_mo            = source_ue->get_rrc_ue()->get_serving_cell_mo();

    CORO_AWAIT_VALUE(target_ue_context_setup_response,
                     target_du_f1ap_ue_ctxt_mng.handle_ue_context_setup_request(target_ue_context_setup_request,
                                                                                source_rrc_context));

    // Handle UE Context Setup Response.
    if (!handle_context_setup_response(response_msg,
                                       bearer_context_modification_request,
                                       target_ue_context_setup_response,
                                       next_config,
                                       logger,
                                       true)) {
      logger.warning("ue={}: \"{}\" failed to create UE context at target DU", request.source_ue_index, name());
      CORO_AWAIT(cu_cp_handler.handle_ue_removal_request(target_ue_context_setup_request.ue_index));
      CORO_EARLY_RETURN(response_msg);
    }

    // Store bearer context modification request for later use in cho_target_routine.
    // This contains the new DL F1-U tunnel endpoints that CU-UP needs after CHO completion.
    response_msg.cho_preparation_result.emplace();
    response_msg.cho_preparation_result->ng_ran_bearer_context_mod_request =
        std::move(bearer_context_modification_request.ng_ran_bearer_context_mod_request);

    logger.debug("ue={} target_ue={}: Stored E1AP bearer context modification for CHO completion",
                 request.source_ue_index,
                 target_ue_context_setup_response.ue_index);

    // Target UE object exists from this point on.
    target_ue = ue_mng.find_du_ue(target_ue_context_setup_response.ue_index);
    ocudu_assert(target_ue != nullptr, "Couldn't find ue={} in target DU", target_ue_context_setup_response.ue_index);

    // Setup SRBs and initialize security context in RRC.
    for (const auto& srb_id : source_rrc_context.srbs) {
      create_srb(target_ue, srb_id);
    }

    srbs_to_setup_for_rrc     = &target_ue_context_setup_request.srbs_to_be_setup_list;
    du_to_cu_rrc_info_for_rrc = &target_ue_context_setup_response.du_to_cu_rrc_info;
    cleanup_target_ue_on_fail = true;
    cleanup_target_ue_index   = target_ue_context_setup_request.ue_index;
  } else {
    // Intra-DU CHO: prepare candidate via UE CONTEXT MODIFICATION on source UE.
    source_ue_context_mod_request.ue_index   = request.source_ue_index;
    source_ue_context_mod_request.sp_cell_id = request.cgi;
    source_ue_context_mod_request.cu_to_du_rrc_info.emplace();
    source_ue_context_mod_request.cu_to_du_rrc_info->ie_exts.emplace();
    source_ue_context_mod_request.cu_to_du_rrc_info->ue_cap_rat_container_list =
        source_rrc_context.ue_cap_rat_container_list.copy();
    if (source_rrc_context.handover_preparation_info.empty()) {
      logger.warning("ue={}: \"{}\" failed to copy HandoverPreparationInformation for intra-DU CHO",
                     request.source_ue_index,
                     name());
      CORO_EARLY_RETURN(response_msg);
    }
    source_ue_context_mod_request.cu_to_du_rrc_info->ie_exts->ho_prep_info =
        source_rrc_context.handover_preparation_info.copy();
    source_ue_context_mod_request.conditional_intra_du_mobility_info.emplace();
    source_ue_context_mod_request.conditional_intra_du_mobility_info->cho_trigger =
        f1ap_cho_trigger_intra_du::cho_initiation;

    CORO_AWAIT_VALUE(source_ue_context_mod_response,
                     source_du_f1ap_ue_ctxt_mng.handle_ue_context_modification_request(source_ue_context_mod_request));

    if (!source_ue_context_mod_response.success) {
      logger.warning("ue={}: \"{}\" failed to prepare intra-DU CHO candidate", request.source_ue_index, name());
      CORO_EARLY_RETURN(response_msg);
    }

    target_ue                 = source_ue;
    du_to_cu_rrc_info_for_rrc = &source_ue_context_mod_response.du_to_cu_rrc_info;
    response_msg.cho_preparation_result.emplace();
    response_msg.cho_preparation_result->target_ue_index = request.source_ue_index;
  }

  // Prepare RRC Reconfiguration (but don't send it yet, keep it for CHO execution).
  if (!fill_rrc_reconfig_args(rrc_reconfig_args,
                              *srbs_to_setup_for_rrc,
                              next_config.pdu_sessions_to_setup_list,
                              {} /* No DRB to be removed */,
                              *du_to_cu_rrc_info_for_rrc,
                              {} /* No NAS PDUs required */,
                              target_ue->get_rrc_ue()->generate_meas_config(source_rrc_context.meas_cfg),
                              true, /* Reestablish SRBs */
                              true /* Reestablish DRBs */,
                              target_ue->get_security_manager().get_ncc(), /* Update keys */
                              target_cell_sib1.copy(),
                              std::nullopt,
                              logger)) {
    logger.warning("ue={}: \"{}\" Failed to fill RrcReconfiguration", request.source_ue_index, name());
    if (cleanup_target_ue_on_fail) {
      CORO_AWAIT(cu_cp_handler.handle_ue_removal_request(cleanup_target_ue_index));
    }
    CORO_EARLY_RETURN(response_msg);
  }

  // Get the packed RRC Reconfiguration for this CHO candidate.
  rrc_reconfig_args.is_cho_preparation = true;
  cho_cand_ctxt = source_ue->get_rrc_ue()->get_rrc_ue_handover_reconfiguration_context(rrc_reconfig_args);

  if (cho_cand_ctxt.rrc_ue_handover_reconfiguration_pdu.empty()) {
    logger.warning(
        "ue={}: \"{}\" Failed to get CHO candidate reconfiguration context", request.source_ue_index, name());
    if (cleanup_target_ue_on_fail) {
      CORO_AWAIT(cu_cp_handler.handle_ue_removal_request(cleanup_target_ue_index));
    }
    CORO_EARLY_RETURN(response_msg);
  }

  // Store the plain ASN.1 RRC Reconfiguration and transaction ID in the response.
  if (!response_msg.cho_preparation_result.has_value()) {
    response_msg.cho_preparation_result.emplace();
  }
  response_msg.cho_preparation_result->packed_rrc_recfg = cho_cand_ctxt.rrc_ue_handover_reconfiguration_pdu.copy();
  response_msg.cho_preparation_result->transaction_id   = cho_cand_ctxt.transaction_id;

  if (!is_intra_du) {
    // Only inter-DU preparation creates a target UE context with UP resources to update.
    up_config_update_result result;
    for (const auto& pdu_session_to_add : next_config.pdu_sessions_to_setup_list) {
      result.pdu_sessions_added_list.push_back(pdu_session_to_add.second);
    }
    target_ue->get_up_resource_manager().apply_config_update(result);
    response_msg.cho_preparation_result->target_ue_index = target_ue_context_setup_response.ue_index;
  }

  response_msg.success = true;

  CORO_RETURN(response_msg);
}
