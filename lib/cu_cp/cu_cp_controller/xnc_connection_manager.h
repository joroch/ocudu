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

#include "../xnap_repository.h"
#include "ocudu/cu_cp/common_task_scheduler.h"
#include "ocudu/cu_cp/cu_cp_xnc_handler.h"

namespace ocudu::ocucp {

class cu_cp_routine_manager;
struct cu_cp_configuration;

class xnc_connection_manager : public cu_cp_xnc_handler
{
public:
  xnc_connection_manager(xnap_repository&       xnaps_,
                         timer_manager&         timers_,
                         task_executor&         cu_cp_exec_,
                         common_task_scheduler& common_task_sched_);

  /// TODO.
  void connect_to_neighbours();

  std::unique_ptr<xnap_message_notifier>
  handle_new_xnc_connection(std::unique_ptr<xnap_message_notifier> xnap_tx_pdu_notifier) override;

  void stop();

private:
  void        handle_connection_setup_result(amf_index_t amf_index, bool success);
  amf_index_t plmn_to_amf_index(plmn_identity plmn) const;

  xnap_repository&        xnaps;
  timer_manager&          timers;
  task_executor&          cu_cp_exec;
  common_task_scheduler&  common_task_sched;
  ocudulog::basic_logger& logger;

  std::unordered_map<xnc_peer_index_t, std::atomic<bool>> xnaps_connected;

  std::atomic<bool> stopped{false};
};

} // namespace ocudu::ocucp
