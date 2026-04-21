// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/fapi/p7/p7_requests_gateway.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/slot_point_extended.h"
#include <cstdint>
#include <map>

namespace ocudu {

namespace fapi {
class p7_slot_indication_notifier;
} // namespace fapi

namespace fapi_adaptor {

class fapi_dummy_ue_simulator;

/// \brief Dummy FAPI P7 requests gateway.
///
/// Routes UL_TTI.request PDUs to the UE simulator for later ACK generation.
/// DL_TTI, UL_DCI, and Tx_Data requests are discarded — the dummy PHY has no real downlink.
/// On each new slot, drives the slot indication notifier to mimic PHY timing.
class fapi_dummy_p7_gateway : public fapi::p7_requests_gateway
{
public:
  /// Wires the slot indication notifier that receives on_slot_indication() calls.
  void set_slot_indication_notifier(fapi::p7_slot_indication_notifier& n) { slot_notifier = &n; }

  /// Wires the UE simulator that buffers UL PDUs and generates ACK responses.
  void set_ue_simulator(fapi_dummy_ue_simulator& sim) { ue_sim = &sim; }

  /// Called by the timing handler each new slot. Fires the slot indication if a notifier is set.
  void on_new_slot(slot_point_extended slot);

  /// Returns true once a slot_indication notifier has been wired via set_slot_indication_notifier().
  bool has_slot_notifier() const { return slot_notifier != nullptr; }

  // fapi::p7_requests_gateway
  void send_dl_tti_request(const fapi::dl_tti_request& msg) override;
  void send_ul_tti_request(const fapi::ul_tti_request& msg) override;
  void send_ul_dci_request(const fapi::ul_dci_request&) override {}
  void send_tx_data_request(const fapi::tx_data_request& msg) override;

private:
  fapi::p7_slot_indication_notifier* slot_notifier = nullptr;
  fapi_dummy_ue_simulator*           ue_sim        = nullptr;
  /// Maps PDSCH PDU index (from DL_TTI.request) to RNTI for the current slot.
  /// Cleared at the start of each send_dl_tti_request call.
  std::map<uint16_t, rnti_t>         pending_dl_pdu_rnti;
};

} // namespace fapi_adaptor
} // namespace ocudu
