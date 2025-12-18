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

#include "../ngap_repository.h"
#include "../ue_manager/ue_manager_impl.h"
#include "ocudu/cu_cp/common_task_scheduler.h"
#include "ocudu/cu_cp/cu_cp.h"
#include "ocudu/ran/plmn_identity.h"
#include <future>

namespace ocudu {
namespace ocucp {

class cu_cp_routine_manager;
struct cu_cp_configuration;

class xnc_connection_manager
{
public:
  xnc_connection_manager(xnap_repository&       ngaps_,
                         timer_manager&         timers_,
                         task_executor&         cu_cp_exec_,
                         common_task_scheduler& common_task_sched_);

  /// TODO.
  void connect_to_neighbours();

  void stop();

private:
  void        handle_connection_setup_result(amf_index_t amf_index, bool success);
  amf_index_t plmn_to_amf_index(plmn_identity plmn) const;

  xnap_repository&        xnaps;
  timer_manager&          timers;
  task_executor&          cu_cp_exec;
  common_task_scheduler&  common_task_sched;
  ocudulog::basic_logger& logger;

  std::unordered_map<amf_index_t, std::atomic<bool>> amfs_connected;

  std::atomic<bool> stopped{false};
};

} // namespace ocucp
} // namespace ocudu
