// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "fapi_dummy_p7_gateway.h"
#include "fapi_dummy_ue_simulator.h"
#include "ocudu/fapi/p7/messages/dl_tti_request.h"
#include "ocudu/fapi/p7/messages/dl_pdsch_pdu.h"
#include "ocudu/fapi/p7/messages/slot_indication.h"
#include "ocudu/fapi/p7/messages/tx_data_request.h"
#include "ocudu/fapi/p7/messages/ul_tti_request.h"
#include "ocudu/fapi/p7/p7_slot_indication_notifier.h"
#include <chrono>

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

void fapi_dummy_p7_gateway::on_new_slot(slot_point_extended slot)
{
  if (slot_notifier != nullptr) {
    slot_notifier->on_slot_indication({slot, std::chrono::system_clock::now()});
  }
}

void fapi_dummy_p7_gateway::send_dl_tti_request(const fapi::dl_tti_request& msg)
{
  if (ue_sim == nullptr) {
    return;
  }
  pending_dl_pdu_rnti.clear();
  // TX_DATA.request uses a PDSCH-only sequential pdu_index (0, 1, 2, ...) that does not
  // count PDCCH/SSB/CSI-RS entries.  Use a separate PDSCH counter so the lookup in
  // send_tx_data_request matches.
  uint16_t pdsch_idx = 0;
  for (const auto& pdu_wrapper : msg.pdus) {
    if (const auto* pdsch = std::get_if<fapi::dl_pdsch_pdu>(&pdu_wrapper.pdu)) {
      pending_dl_pdu_rnti[pdsch_idx] = pdsch->rnti;
      ue_sim->on_dl_pdsch_for_rnti(pdsch->rnti);
      ++pdsch_idx;
    }
  }
}

void fapi_dummy_p7_gateway::send_tx_data_request(const fapi::tx_data_request& msg)
{
  if (ue_sim == nullptr) {
    return;
  }
  for (const auto& pdu : msg.pdus) {
    auto it = pending_dl_pdu_rnti.find(pdu.pdu_index);
    if (it == pending_dl_pdu_rnti.end()) {
      continue;
    }
    auto buf = pdu.pdu.get_buffer();
    ue_sim->on_dl_srb1_pdu(it->second, buf.data(), buf.size());
  }
}

void fapi_dummy_p7_gateway::send_ul_tti_request(const fapi::ul_tti_request& msg)
{
  if (ue_sim != nullptr) {
    ue_sim->store_ul_tti(msg);
  }
}
