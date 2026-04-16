// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "fapi_dummy_sector.h"

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

fapi_dummy_sector::fapi_dummy_sector(const fapi_dummy_cell_config& cfg_) : cfg(cfg_), ue_sim(cfg_.ue)
{
  p7_gw.set_ue_simulator(ue_sim);
}

void fapi_dummy_sector::on_new_slot(slot_point_extended slot)
{
  p7_gw.on_new_slot(slot);
  // Drive the UE simulator only after the MAC cell processor is fully wired.
  // Before o_du_high_impl::start() calls set_cell_rach_handler(), any RACH.indication
  // fired by the simulator would reach a dummy handler and be silently dropped, causing
  // neither UE to ever attach.  mac_started becomes true on the first on_last_message()
  // call, which arrives only after the MAC cell processor processes its first slot.
  if (p7_last_req.mac_started.load(std::memory_order_acquire)) {
    ue_sim.on_new_slot(slot.without_hyper_sfn());
  }
}

bool fapi_dummy_sector::try_on_new_slot(slot_point_extended slot)
{
  // Credit control has two gates:
  //  1. The P7 slot_indication notifier must be wired (has_slot_notifier()).
  //     Before wiring, slot indications are no-ops, so no credits should be taken.
  //  2. The MAC cell processor must be fully started (mac_started flag).
  //     During the P5 startup phase, slot_indications are consumed by p5_responses_handler
  //     only; the real MAC cell processor is not yet wired, so on_last_message never fires.
  //     Applying credit control here would deadlock: credits fill up and the very
  //     slot_indication needed to complete P5 START never arrives.
  if (p7_gw.has_slot_notifier() && p7_last_req.mac_started.load(std::memory_order_acquire)) {
    if (p7_last_req.slots_in_flight.load(std::memory_order_acquire) >= p7_last_request_notifier_impl::max_in_flight) {
      return false;
    }
    p7_last_req.slots_in_flight.fetch_add(1, std::memory_order_relaxed);
  }
  on_new_slot(slot);
  return true;
}
