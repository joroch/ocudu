// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "fapi_dummy_p7_gateway.h"
#include "fapi_dummy_ue_simulator.h"
#include "ocudu/fapi/p7/messages/slot_indication.h"
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

void fapi_dummy_p7_gateway::send_ul_tti_request(const fapi::ul_tti_request& msg)
{
  if (ue_sim != nullptr) {
    ue_sim->store_ul_tti(msg);
  }
}
