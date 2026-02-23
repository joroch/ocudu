/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "mobility_helpers.h"
#include "../../ue_manager/cu_cp_ue_impl.h"
#include "../pdu_session_routine_helpers.h"

using namespace ocudu;
using namespace ocudu::ocucp;

bool ocudu::ocucp::fill_ue_context_setup_request(f1ap_ue_context_setup_request&               setup_request,
                                                 nr_cell_global_id_t                          cgi,
                                                 const static_vector<srb_id_t, MAX_NOF_SRBS>& srbs,
                                                 const rrc_ue_transfer_context&               transfer_context,
                                                 const up_config_update&                      next_config,
                                                 bool                                         is_cho)
{
  setup_request.serv_cell_idx = 0; // TODO: Remove hardcoded value
  setup_request.sp_cell_id    = cgi;

  if (transfer_context.handover_preparation_info.empty()) {
    return false;
  }

  setup_request.cu_to_du_rrc_info.ie_exts.emplace();
  auto buffer_copy = transfer_context.handover_preparation_info.deep_copy();
  if (!buffer_copy) {
    return false;
  }
  setup_request.cu_to_du_rrc_info.ie_exts.value().ho_prep_info = std::move(buffer_copy.value());
  setup_request.cu_to_du_rrc_info.ue_cap_rat_container_list    = transfer_context.ue_cap_rat_container_list.copy();

  if (is_cho) {
    setup_request.conditional_inter_du_mobility_info.emplace();
    setup_request.conditional_inter_du_mobility_info->cho_trigger = f1ap_cho_trigger::cho_initiation;
  }

  for (const auto& srb_id : srbs) {
    f1ap_srb_to_setup srb_item;
    srb_item.srb_id = srb_id;
    setup_request.srbs_to_be_setup_list.push_back(srb_item);
  }

  for (const auto& pdu_session : next_config.pdu_sessions_to_setup_list) {
    for (const auto& drb : pdu_session.second.drb_to_add) {
      const up_drb_context& drb_context = drb.second;

      f1ap_drb_to_setup drb_item;
      drb_item.drb_id           = drb_context.drb_id;
      drb_item.qos_info.drb_qos = drb_context.qos_params;

      // Add each QoS flow including QoS.
      for (const auto& flow : drb_context.qos_flows) {
        flow_mapped_to_drb flow_item;
        flow_item.qos_flow_id               = flow.first;
        flow_item.qos_flow_level_qos_params = flow.second.qos_params;
        drb_item.qos_info.flows_mapped_to_drb_list.push_back(flow_item);
      }
      drb_item.uluptnl_info_list = drb_context.ul_up_tnl_info_to_be_setup_list;
      drb_item.mode              = drb_context.rlc_mod;
      drb_item.pdcp_sn_len       = drb_context.pdcp_cfg.tx.sn_size;

      setup_request.drbs_to_be_setup_list.push_back(drb_item);
    }
  }

  return true;
}

void ocudu::ocucp::create_srb(cu_cp_ue* ue, srb_id_t srb_id)
{
  srb_creation_message srb_msg{};
  srb_msg.ue_index        = ue->get_ue_index();
  srb_msg.srb_id          = srb_id;
  srb_msg.enable_security = true;
  // TODO: add support for non-default PDCP config.
  ue->get_rrc_ue()->create_srb(srb_msg);
}

bool ocudu::ocucp::handle_context_setup_response(
    cu_cp_intra_cu_handover_response&         response_msg,
    e1ap_bearer_context_modification_request& bearer_context_modification_request,
    const f1ap_ue_context_setup_response&     target_ue_context_setup_response,
    up_config_update&                         next_config,
    const ocudulog::basic_logger&             logger,
    bool                                      reestablish_pdcp)
{
  // Sanity checks.
  if (target_ue_context_setup_response.ue_index == ue_index_t::invalid) {
    logger.warning("Failed to create UE at the target DU");
    return false;
  }

  if (!target_ue_context_setup_response.srbs_failed_to_be_setup_list.empty()) {
    logger.warning("Couldn't setup {} SRBs at target DU",
                   target_ue_context_setup_response.srbs_failed_to_be_setup_list.size());
    return false;
  }

  if (!target_ue_context_setup_response.drbs_failed_to_be_setup_list.empty()) {
    logger.warning("Couldn't setup {} DRBs at target DU",
                   target_ue_context_setup_response.drbs_failed_to_be_setup_list.size());
    return false;
  }

  if (!target_ue_context_setup_response.c_rnti.has_value()) {
    logger.warning("No C-RNTI present in UE context setup");
    return false;
  }

  // Create bearer context mod request.
  if (!target_ue_context_setup_response.drbs_setup_list.empty()) {
    auto& context_mod_request = bearer_context_modification_request.ng_ran_bearer_context_mod_request.emplace();

    // Extract new DL tunnel information for CU-UP.
    for (const auto& pdu_session : next_config.pdu_sessions_to_setup_list) {
      // The modifications are only for this PDU session.
      e1ap_pdu_session_res_to_modify_item e1ap_mod_item;
      e1ap_mod_item.pdu_session_id = pdu_session.first;

      for (const auto& drb_item : pdu_session.second.drb_to_add) {
        auto drb_it = std::find_if(target_ue_context_setup_response.drbs_setup_list.begin(),
                                   target_ue_context_setup_response.drbs_setup_list.end(),
                                   [&drb_item](const auto& drb) { return drb.drb_id == drb_item.first; });
        ocudu_assert(drb_it != target_ue_context_setup_response.drbs_setup_list.end(),
                     "Couldn't find {} in UE context setup response",
                     drb_item.first);
        const auto& context_setup_drb_item = *drb_it;

        e1ap_drb_to_modify_item_ng_ran e1ap_drb_item;
        e1ap_drb_item.drb_id = drb_item.first;

        for (const auto& dl_up_tnl_info : context_setup_drb_item.dluptnl_info_list) {
          e1ap_up_params_item e1ap_dl_up_param;
          e1ap_dl_up_param.up_tnl_info   = dl_up_tnl_info;
          e1ap_dl_up_param.cell_group_id = 0; // TODO: Remove hardcoded value

          e1ap_drb_item.dl_up_params.push_back(e1ap_dl_up_param);
        }

        if (reestablish_pdcp) {
          // Reestablish PDCP.
          e1ap_drb_item.pdcp_cfg.emplace();
          fill_e1ap_drb_pdcp_config(e1ap_drb_item.pdcp_cfg.value(), drb_item.second.pdcp_cfg);
          e1ap_drb_item.pdcp_cfg->pdcp_reest = true;
        }

        e1ap_mod_item.drb_to_modify_list_ng_ran.emplace(e1ap_drb_item.drb_id, e1ap_drb_item);
      }

      context_mod_request.pdu_session_res_to_modify_list.emplace(e1ap_mod_item.pdu_session_id, e1ap_mod_item);
    }
  }

  return target_ue_context_setup_response.success;
}

bool ocudu::ocucp::handle_bearer_context_modification_response(
    cu_cp_intra_cu_handover_response&                response_msg,
    f1ap_ue_context_modification_request&            source_ue_context_mod_request,
    const e1ap_bearer_context_modification_response& bearer_context_modification_response,
    up_config_update&                                next_config,
    const ocudulog::basic_logger&                    logger)

{
  // TOOD: Add proper handling.
  return bearer_context_modification_response.success;
}
