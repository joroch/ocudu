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
#include "ocudu/adt/span.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace ocucp {

/// \brief Handles cancellation of Conditional Handover (CHO).
///
/// Behaviour depends on the CHO state at invocation time:
///
/// Steps:
///   1. Select candidates to cancel (all or by cond_recfg_id).
///   2. If needed, send source-side UE Context Modification:
///      - include RRC condReconfigToRemoveList if CHO reconfiguration was already sent to UE,
///      - include intra-DU CHO-cancel info (target cells) when intra-DU candidates are being cancelled.
///   3. Release inter-DU target UE contexts.
///   4. Update CHO context state.
class cho_cancellation_routine
{
public:
  cho_cancellation_routine(const cu_cp_cho_cancellation_request& request_,
                           cu_cp_ue&                             source_ue_,
                           f1ap_ue_context_manager&              source_du_f1ap_ue_ctxt_mng_,
                           cu_cp_ue_context_release_handler&     ue_context_release_handler_,
                           ue_manager&                           ue_mng_,
                           ocudulog::basic_logger&               logger_);

  void operator()(coro_context<async_task<cu_cp_cho_cancellation_response>>& ctx);

  static const char* name() { return "CHO Cancellation Routine"; }

private:
  /// \brief Build the RRC message with condReconfigToRemoveList.
  /// \param[in] cond_recfg_ids IDs to remove.
  /// \return Packed RRC message on success, empty buffer on failure.
  byte_buffer build_cho_removal_message(span<const cond_recfg_id_t> cond_recfg_ids);

  /// \brief Generate the F1AP UE Context Modification request.
  void generate_ue_context_modification_request(bool                            include_rrc_container,
                                                span<const nr_cell_global_id_t> intra_du_cells_to_cancel);

  const cu_cp_cho_cancellation_request request;

  cu_cp_ue*                         source_ue;       // re-fetched after each CORO_AWAIT; null means UE was removed
  ue_index_t                        source_ue_index; // value-type, stays valid after UE removal (used for logging)
  f1ap_ue_context_manager&          source_du_f1ap_ue_ctxt_mng;
  cu_cp_ue_context_release_handler& ue_context_release_handler;
  ue_manager&                       ue_mng;
  ocudulog::basic_logger&           logger;

  // Response
  cu_cp_cho_cancellation_response response_msg;

  // RRC reconfiguration context
  uint8_t     transaction_id = 0;
  byte_buffer rrc_container;

  // (sub-)routine requests
  f1ap_ue_context_modification_request ue_context_mod_request;
  cu_cp_ue_context_release_command     candidate_release_command;

  // (sub-)routine results
  f1ap_ue_context_modification_response ue_context_mod_response;
  bool                                  reconfig_result = false;

  // Coroutine state variables
  bool                             rrc_was_sent                 = false;
  bool                             needs_source_du_modification = false;
  std::vector<cond_recfg_id_t>     cond_recfg_ids_to_remove;
  std::vector<nr_cell_global_id_t> intra_du_cells_to_cancel;
  std::vector<ue_index_t>          inter_du_targets_to_release;
  std::vector<cu_cp_cho_candidate> remaining_candidates;
  size_t                           release_idx = 0;
};

} // namespace ocucp
} // namespace ocudu
