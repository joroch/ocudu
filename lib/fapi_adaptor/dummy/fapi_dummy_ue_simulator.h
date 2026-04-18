// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "fapi_dummy_config.h"
#include "ocudu/fapi/p7/messages/ul_pucch_pdu.h"
#include "ocudu/fapi/p7/messages/ul_pusch_pdu.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/security/security_engine.h"
#include <array>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace ocudu {

namespace fapi {
class p7_indications_notifier;
struct ul_tti_request;
} // namespace fapi

namespace fapi_adaptor {

/// \brief Simulates UE uplink behaviour at the FAPI boundary.
///
/// On each slot this class:
///  1. Fires a rach_indication for the next simulated UE if the stagger interval has elapsed.
///  2. Pops any buffered UL_TTI.request PDUs for this slot and generates:
///       - CRC.indication          (tb_crc_status_ok = true)     for every PUSCH PDU with data
///       - Rx_Data.indication      (zero-filled transport block)  for every PUSCH PDU with data
///       - UCI.indication           (all HARQ bits = ACK)          for every PUCCH/PUSCH-UCI PDU
///
/// The number of simulated UEs and the per-UE creation stagger interval are configured via
/// fapi_dummy_ue_config (passed at construction, defaults to nof_ues=0 / no RACH).
class fapi_dummy_ue_simulator
{
public:
  explicit fapi_dummy_ue_simulator(const fapi_dummy_ue_config& cfg = {});

  /// Wires the indications notifier that receives the generated messages.
  void set_notifier(fapi::p7_indications_notifier& n) { notifier = &n; }

  /// Stores UL PDUs from a UL_TTI.request for later processing.
  void store_ul_tti(const fapi::ul_tti_request& msg);

  /// Fires any pending RACH indication and generates UL ACK responses for \p slot.
  void on_new_slot(slot_point slot);

  /// Fires UL ACK responses for any UL_TTI stored at the given slot index.
  ///
  /// Called after the MAC has finished processing a slot so that the UL_TTI stored
  /// during that slot's processing can be ACK'd with the correct UL transmission slot.
  void flush_ul(slot_point slot);

private:
  /// Attach phase for a single UE, tracked per C-RNTI.
  enum class ue_ul_state : uint8_t {
    ccch_pending, ///< First PUSCH → Msg3 (RRCSetupRequest) on CCCH (LCID=0).
    srb1_pending, ///< Msg3 sent; next sufficiently large PUSCH → rrcSetupComplete on SRB1 (LCID=1).
    srb1_sent,    ///< rrcSetupComplete sent; waiting for first DL PDSCH (SecurityModeCommand) before sending SMC.
    smc_pending,  ///< DL PDSCH seen; next PUSCH → SecurityModeComplete on SRB1 (LCID=1).
    registered,   ///< SecurityModeComplete sent; all subsequent PUSCH responses are padding.
  };

  /// Per-slot storage for buffered UL PDUs.
  struct slot_data {
    bool                            valid = false;
    slot_point                      ul_slot;   ///< original UL TTI transmission slot (from UL_TTI.request)
    std::vector<fapi::ul_pusch_pdu> pusch_pdus;
    std::vector<fapi::ul_pucch_pdu> pucch_pdus;
  };

  /// Buffer size — enough for a full radio frame at SCS 30 kHz (20 slots).
  static constexpr unsigned BUFFER_SIZE = 32;

  /// Sentinel meaning "no RACH has been scheduled yet" — set to the first slot seen.
  static constexpr uint32_t RACH_SLOT_UNSET = std::numeric_limits<uint32_t>::max();

  void maybe_fire_rach(slot_point slot);
  void process_pusch(slot_point slot, const fapi::ul_pusch_pdu& pdu);
  void process_pucch(slot_point slot, const fapi::ul_pucch_pdu& pdu);

public:
  /// Notifies the UE simulator that a DL PDSCH was scheduled for \p rnti.
  /// Used to detect SecurityModeCommand and transition srb1_sent → smc_pending.
  void on_dl_pdsch_for_rnti(rnti_t rnti);

  void init_rrc_security();

  fapi_dummy_ue_config                             cfg;
  fapi::p7_indications_notifier*                   notifier       = nullptr;
  unsigned                                         next_ue_index  = 0;
  uint32_t                                         next_rach_slot = RACH_SLOT_UNSET;
  std::array<slot_data, BUFFER_SIZE>               buffer{};
  std::map<rnti_t, ue_ul_state>                    rnti_states;
  std::set<rnti_t>                                 srb1_status_needed; ///< RNTIs that must send RLC STATUS PDU next UL.
  std::unique_ptr<security::security_engine_tx>    rrc_sec_engine;
};

} // namespace fapi_adaptor
} // namespace ocudu
