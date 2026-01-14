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

#include "adapters/xnap_adapters.h"
#include "ocudu/cu_cp/cu_cp_configuration.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/cu_cp/cu_cp_xnc_handler.h"
#include "ocudu/xnap/xnap.h"
#include "ocudu/xnap/xnap_message_notifier.h"

namespace ocudu::ocucp {

struct cu_cp_configuration;

struct xnap_repository_config {
  const cu_cp_configuration& cu_cp;
  ocudulog::basic_logger&    logger;
};

class xnap_repository
{
public:
  explicit xnap_repository(xnap_repository_config cfg_);

  /// \brief Adds a XNAP object to the CU-CP.
  /// \return A pointer to the interface of the added NGAP object if it was successfully created, a nullptr otherwise.
  xnap_interface* add_xnap(xnc_peer_index_t xnc_index, const cu_cp_configuration::xnap_config& config);

  /// \brief Get the all NGAP interfaces.
  std::map<xnc_peer_index_t, xnap_interface*> get_xnaps();

  // ngap_task_scheduler& get_ngap_task_scheduler() { return amf_task_sched; }

  /// Number of NGAPs managed by the CU-CP.
  size_t get_nof_xnaps() const { return xnap_db.size(); }

  /// Number of UEs managed by the CU-CP.
  size_t get_nof_ngap_ues();

private:
  struct xnap_context {
    // Peer address.
    std::string peer_addr;
    // CU-CP handler of XNAP events.
    xnap_cu_cp_adapter xnap_to_cu_cp_notifier;

    std::unique_ptr<xnap_interface> xnap;

    /// Notifier used by the CU-CP to push NGAP Tx messages to the respective AMF.
    std::unique_ptr<xnap_message_notifier> xnap_tx_pdu_notifier;
  };

  xnap_repository_config  cfg;
  ocudulog::basic_logger& logger;

  // ngap_task_scheduler amf_task_sched;

  std::map<xnc_peer_index_t, xnap_context> xnap_db;
};

} // namespace ocudu::ocucp
