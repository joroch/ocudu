// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/fapi/p7/messages/ul_pucch_pdu.h"
#include "ocudu/fapi/p7/messages/ul_pusch_pdu.h"
#include "ocudu/ran/slot_point.h"
#include <array>
#include <vector>

namespace ocudu {

namespace fapi {
class p7_indications_notifier;
struct ul_tti_request;
} // namespace fapi

namespace fapi_adaptor {

/// \brief Simulates UE uplink behaviour at the FAPI boundary.
///
/// Buffers UL_TTI.request PDUs per slot and, when on_new_slot() fires, generates:
///  - CRC.indication          (tb_crc_status_ok = true)  for every PUSCH PDU with data
///  - Rx_Data.indication      (zero-filled transport block) for every PUSCH PDU with data
///  - UCI.indication           (all HARQ bits = ACK)       for every PUCCH PDU
///
/// PUSCH UCI multiplexed on a PUSCH PDU also generates a UCI.indication.
class fapi_dummy_ue_simulator
{
public:
  /// Wires the indications notifier that receives the generated ACK messages.
  void set_notifier(fapi::p7_indications_notifier& n) { notifier = &n; }

  /// Stores UL PDUs from a UL_TTI.request for later processing.
  void store_ul_tti(const fapi::ul_tti_request& msg);

  /// Generates ACK responses for all buffered UL PDUs matching \p slot.
  void on_new_slot(slot_point slot);

private:
  /// Per-slot storage for buffered UL PDUs.
  struct slot_data {
    bool                         valid = false;
    std::vector<fapi::ul_pusch_pdu> pusch_pdus;
    std::vector<fapi::ul_pucch_pdu> pucch_pdus;
  };

  /// Buffer size — enough for a full radio frame at SCS 30 kHz (20 slots).
  static constexpr unsigned BUFFER_SIZE = 32;

  void process_pusch(slot_point slot, const fapi::ul_pusch_pdu& pdu);
  void process_pucch(slot_point slot, const fapi::ul_pucch_pdu& pdu);

  fapi::p7_indications_notifier*          notifier = nullptr;
  std::array<slot_data, BUFFER_SIZE>      buffer{};
};

} // namespace fapi_adaptor
} // namespace ocudu
