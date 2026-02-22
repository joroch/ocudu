/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cho_cancellation_routine.h"
#include <algorithm>
using namespace ocudu;
using namespace ocudu::ocucp;

cho_cancellation_routine::cho_cancellation_routine(const cu_cp_cho_cancellation_request& request_,
                                                   cu_cp_ue&                             source_ue_,
                                                   f1ap_ue_context_manager&              source_du_f1ap_ue_ctxt_mng_,
                                                   cu_cp_ue_context_release_handler&     ue_context_release_handler_,
                                                   ue_manager&                           ue_mng_,
                                                   ocudulog::basic_logger&               logger_) :
  request(request_),
  source_ue(&source_ue_),
  source_ue_index(source_ue_.get_ue_index()),
  source_du_f1ap_ue_ctxt_mng(source_du_f1ap_ue_ctxt_mng_),
  ue_context_release_handler(ue_context_release_handler_),
  ue_mng(ue_mng_),
  logger(logger_)
{
  ocudu_assert(source_ue_index != ue_index_t::invalid, "Invalid source UE index {}", source_ue_index);
}

void cho_cancellation_routine::operator()(coro_context<async_task<cu_cp_cho_cancellation_response>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.debug("ue={}: \"{}\" started...", source_ue_index, name());

  // Validate CHO context.
  {
    auto& cho_ctx = source_ue->get_cho_context();
    if (!cho_ctx.has_value() || cho_ctx->state == cu_cp_ue_cho_context::state_t::idle) {
      logger.warning("ue={}: \"{}\" failed. No CHO configured", source_ue_index, name());
      CORO_EARLY_RETURN(response_msg);
    }

    if (cho_ctx->candidates.empty()) {
      logger.warning("ue={}: \"{}\" failed. No candidates to cancel", source_ue_index, name());
      CORO_EARLY_RETURN(response_msg);
    }
  }

  // Determine whether the conditional reconfiguration was already sent to the UE.
  // targets_preparation -> no RRC was sent; only target UE contexts need releasing.
  // rrc_reconfiguration / execution / completion -> condReconfiguration-r16 may have been delivered;
  //                                                  send condReconfigToRemoveList and await completion before cleanup.
  {
    auto& cho_ctx = source_ue->get_cho_context();
    rrc_was_sent  = (cho_ctx->state == cu_cp_ue_cho_context::state_t::rrc_reconfiguration ||
                    cho_ctx->state == cu_cp_ue_cho_context::state_t::execution ||
                    cho_ctx->state == cu_cp_ue_cho_context::state_t::completion);
  }

  // Split selected candidates into:
  // - inter-DU candidates that need target UE context release.
  // - intra-DU candidates that need source DU CHO-cancel signaling.
  {
    const auto& cho_ctx = source_ue->get_cho_context().value();
    for (const auto& candidate : cho_ctx.candidates) {
      const bool selected = request.cond_recfg_ids_to_remove.empty() ||
                            std::find(request.cond_recfg_ids_to_remove.begin(),
                                      request.cond_recfg_ids_to_remove.end(),
                                      candidate.cond_recfg_id) != request.cond_recfg_ids_to_remove.end();
      if (!selected) {
        remaining_candidates.push_back(candidate);
        continue;
      }

      cond_recfg_ids_to_remove.push_back(candidate.cond_recfg_id);
      if (candidate.target_ue_index == source_ue_index) {
        intra_du_cells_to_cancel.push_back(candidate.target_cgi);
      } else {
        if (std::find(inter_du_targets_to_release.begin(),
                      inter_du_targets_to_release.end(),
                      candidate.target_ue_index) == inter_du_targets_to_release.end()) {
          inter_du_targets_to_release.push_back(candidate.target_ue_index);
        }
      }
    }
  }

  if (cond_recfg_ids_to_remove.empty()) {
    logger.warning("ue={}: \"{}\" failed. No matching CHO candidates for cancellation", source_ue_index, name());
    CORO_EARLY_RETURN(response_msg);
  }

  needs_source_du_modification = rrc_was_sent || !intra_du_cells_to_cancel.empty();

  if (needs_source_du_modification) {
    if (rrc_was_sent) {
      // Build RRC condReconfigToRemoveList only when the conditional reconfiguration was already sent to UE.
      rrc_container = build_cho_removal_message(cond_recfg_ids_to_remove);
      if (rrc_container.empty()) {
        logger.warning("ue={}: \"{}\" failed. Could not build CHO removal message", source_ue_index, name());
        CORO_EARLY_RETURN(response_msg);
      }
    }

    // Generate source-side UE Context Modification request.
    generate_ue_context_modification_request(rrc_was_sent, intra_du_cells_to_cancel);

    // Send source-side F1AP UE Context Modification.
    CORO_AWAIT_VALUE(ue_context_mod_response,
                     source_du_f1ap_ue_ctxt_mng.handle_ue_context_modification_request(ue_context_mod_request));

    // Re-check liveness: UE may have been released during the F1AP round-trip.
    source_ue = ue_mng.find_du_ue(source_ue_index);
    if (source_ue == nullptr) {
      logger.warning("ue={}: UE removed during \"{}\", aborting", source_ue_index, name());
      CORO_EARLY_RETURN(response_msg);
    }

    if (!ue_context_mod_response.success) {
      logger.warning("ue={}: \"{}\" failed. UE context modification failed", source_ue_index, name());
      CORO_EARLY_RETURN(response_msg);
    }

    if (rrc_was_sent) {
      // Wait for RRCReconfigurationComplete from the UE using configured timeout only if an RRC removal was sent.
      CORO_AWAIT_VALUE(reconfig_result,
                       source_ue->get_rrc_ue()->handle_handover_reconfiguration_complete_expected(
                           transaction_id, request.rrc_complete_timeout));

      // Re-check liveness: UE may have been released during the RRCReconfig wait.
      source_ue = ue_mng.find_du_ue(source_ue_index);
      if (source_ue == nullptr) {
        logger.warning("ue={}: UE removed during \"{}\", aborting", source_ue_index, name());
        CORO_EARLY_RETURN(response_msg);
      }

      if (!reconfig_result) {
        logger.warning("ue={}: \"{}\" failed. RRCReconfigurationComplete not received", source_ue_index, name());
        CORO_EARLY_RETURN(response_msg);
      }
    }
  } else {
    logger.debug("ue={}: \"{}\" no source-side modification needed (inter-DU prepared-only cancellation)",
                 source_ue_index,
                 name());
  }

  // Release target UE contexts for selected inter-DU candidates only.
  release_idx = 0;
  while (release_idx < inter_du_targets_to_release.size()) {
    logger.debug(
        "ue={}: Releasing CHO candidate target_ue={}", source_ue_index, inter_du_targets_to_release[release_idx]);

    candidate_release_command.ue_index             = inter_du_targets_to_release[release_idx];
    candidate_release_command.cause                = ngap_cause_radio_network_t::unspecified;
    candidate_release_command.requires_rrc_release = false;
    CORO_AWAIT(ue_context_release_handler.handle_ue_context_release_command(candidate_release_command));
    ++release_idx;

    // Re-check liveness: UE may have been released during a candidate release.
    source_ue = ue_mng.find_du_ue(source_ue_index);
    if (source_ue == nullptr) {
      logger.warning("ue={}: UE removed during \"{}\", aborting", source_ue_index, name());
      CORO_EARLY_RETURN(response_msg);
    }
  }

  // Update CHO context only after protocol actions and target releases succeed.
  {
    auto& cho_ctx = source_ue->get_cho_context();
    if (cho_ctx.has_value()) {
      cho_ctx->candidates = std::move(remaining_candidates);
      if (cho_ctx->candidates.empty()) {
        cho_ctx->clear();
      }
    }
  }

  logger.info("ue={}: \"{}\" completed successfully. Cancelled {} candidate(s)",
              source_ue_index,
              name(),
              cond_recfg_ids_to_remove.size());

  response_msg.success = true;
  CORO_RETURN(response_msg);
}

byte_buffer cho_cancellation_routine::build_cho_removal_message(span<const cond_recfg_id_t> cond_recfg_ids)
{
  if (cond_recfg_ids.empty()) {
    return {};
  }

  rrc_reconfiguration_procedure_request rrc_request;
  rrc_request.cond_recfg_ids_to_remove = std::vector<cond_recfg_id_t>(cond_recfg_ids.begin(), cond_recfg_ids.end());

  auto cond_reconf_ctx = source_ue->get_rrc_ue()->get_rrc_ue_cond_reconfiguration_context(rrc_request);
  if (cond_reconf_ctx.rrc_ue_cond_reconfiguration_pdu.empty()) {
    logger.warning("ue={}: Failed to build CHO removal RRCReconfiguration", source_ue_index);
    return {};
  }

  transaction_id = cond_reconf_ctx.transaction_id;

  logger.debug(
      "ue={}: Built CHO removal RRCReconfiguration with {} ID(s) to remove", source_ue_index, cond_recfg_ids.size());

  return std::move(cond_reconf_ctx.rrc_ue_cond_reconfiguration_pdu);
}

void cho_cancellation_routine::generate_ue_context_modification_request(
    bool                            include_rrc_container,
    span<const nr_cell_global_id_t> intra_du_target_cells_to_cancel)
{
  ue_context_mod_request          = {};
  ue_context_mod_request.ue_index = source_ue_index;

  if (include_rrc_container) {
    ue_context_mod_request.rrc_container = rrc_container.copy();
  }

  if (!intra_du_target_cells_to_cancel.empty()) {
    ue_context_mod_request.conditional_intra_du_mobility_info.emplace();
    ue_context_mod_request.conditional_intra_du_mobility_info->cho_trigger = f1ap_cho_trigger_intra_du::cho_cancel;
    ue_context_mod_request.conditional_intra_du_mobility_info->target_cells_to_cancel.assign(
        intra_du_target_cells_to_cancel.begin(), intra_du_target_cells_to_cancel.end());
  }
}
