// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "fapi_dummy_ue_simulator.h"
#include "ocudu/adt/span.h"
#include "ocudu/fapi/p7/messages/crc_indication.h"
#include "ocudu/fapi/p7/messages/rach_indication.h"
#include "ocudu/fapi/p7/messages/rx_data_indication.h"
#include "ocudu/fapi/p7/messages/uci_indication.h"
#include "ocudu/fapi/p7/messages/ul_tti_request.h"
#include "ocudu/fapi/p7/p7_indications_notifier.h"
#include "ocudu/ran/uci/uci_mapping.h"
#include "ocudu/ran/uci/uci_payload_type.h"
#include <limits>
#include <vector>

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

// SINR representative of a strong signal (~30 dB). Encoded as int16_t at 0.002 dB/LSB.
static constexpr int16_t  DUMMY_SINR = 15000;
// Disabled RSSI/RSRP sentinel per SCF-222.
static constexpr uint16_t DISABLED_METRIC = std::numeric_limits<uint16_t>::max();

// RACH power/SNR encoded per SCF-222: rssi_dBm = (fapi_rssi - 140000) * 0.001
//                                      snr_dB   = (fapi_snr  - 128)    * 0.5
static constexpr uint32_t RACH_RSSI = 130'000U; // -10 dBm
static constexpr uint8_t  RACH_SNR  = 188U;     // 30 dB: (30 / 0.5) + 128

fapi_dummy_ue_simulator::fapi_dummy_ue_simulator(const fapi_dummy_ue_config& cfg_) : cfg(cfg_) {}

void fapi_dummy_ue_simulator::store_ul_tti(const fapi::ul_tti_request& msg)
{
  slot_data& entry = buffer[msg.slot.system_slot() % BUFFER_SIZE];
  entry.valid      = true;
  entry.pusch_pdus.clear();
  entry.pucch_pdus.clear();

  for (const auto& pdu_wrapper : msg.pdus) {
    if (const auto* pusch = std::get_if<fapi::ul_pusch_pdu>(&pdu_wrapper.pdu)) {
      entry.pusch_pdus.push_back(*pusch);
    } else if (const auto* pucch = std::get_if<fapi::ul_pucch_pdu>(&pdu_wrapper.pdu)) {
      entry.pucch_pdus.push_back(*pucch);
    }
    // PRACH and SRS PDUs are ignored — handled by separate stages (RACH in Stage 5).
  }
}

void fapi_dummy_ue_simulator::on_new_slot(slot_point slot)
{
  maybe_fire_rach(slot);

  slot_data& entry = buffer[slot.system_slot() % BUFFER_SIZE];
  if (!entry.valid) {
    return;
  }

  // Take ownership and clear the buffer entry before firing callbacks.
  slot_data local_data;
  std::swap(local_data, entry);

  for (const auto& pdu : local_data.pusch_pdus) {
    process_pusch(slot, pdu);
  }
  for (const auto& pdu : local_data.pucch_pdus) {
    process_pucch(slot, pdu);
  }
}

void fapi_dummy_ue_simulator::maybe_fire_rach(slot_point slot)
{
  if (notifier == nullptr || cfg.nof_ues == 0 || next_ue_index >= cfg.nof_ues) {
    return;
  }

  // Initialise the first RACH slot to the first slot seen.
  if (next_rach_slot == RACH_SLOT_UNSET) {
    next_rach_slot = slot.system_slot();
  }

  if (slot.system_slot() < next_rach_slot) {
    return;
  }

  fapi::rach_indication_pdu_preamble preamble{};
  preamble.preamble_index        = static_cast<uint8_t>(next_ue_index % 64U);
  preamble.timing_advance_offset = std::nullopt;
  preamble.preamble_pwr          = RACH_RSSI;
  preamble.preamble_snr          = RACH_SNR;

  fapi::rach_indication ind{};
  ind.slot             = slot;
  ind.pdu.avg_rssi     = RACH_RSSI;
  ind.pdu.avg_snr      = RACH_SNR;
  ind.pdu.symbol_index = 0;
  ind.pdu.slot_index   = 0;
  ind.pdu.ra_index     = 0;
  ind.pdu.preambles.push_back(preamble);

  notifier->on_rach_indication(ind);

  ++next_ue_index;
  next_rach_slot = slot.system_slot() + cfg.ue_creation_stagger_slots;
}

void fapi_dummy_ue_simulator::process_pusch(slot_point slot, const fapi::ul_pusch_pdu& pdu)
{
  if (notifier == nullptr) {
    return;
  }

  // Generate CRC + Rx_Data for the transport block (if present).
  if (pdu.pusch_data.has_value()) {
    const auto& data = *pdu.pusch_data;

    fapi::crc_indication crc{};
    crc.slot                    = slot;
    crc.pdu.handle              = pdu.handle;
    crc.pdu.rnti                = pdu.rnti;
    crc.pdu.harq_id             = data.harq_process_id;
    crc.pdu.tb_crc_status_ok    = true;
    crc.pdu.ul_sinr_metric      = DUMMY_SINR;
    crc.pdu.rssi                = DISABLED_METRIC;
    crc.pdu.rsrp                = DISABLED_METRIC;
    notifier->on_crc_indication(crc);

    // Zero-filled transport block — the span is valid during the callback.
    std::vector<uint8_t> zeros(data.tb_size.value(), 0);
    fapi::rx_data_indication rx{};
    rx.slot               = slot;
    rx.pdu.handle         = pdu.handle;
    rx.pdu.rnti           = pdu.rnti;
    rx.pdu.harq_id        = data.harq_process_id;
    rx.pdu.transport_block = span<const uint8_t>(zeros.data(), zeros.size());
    notifier->on_rx_data_indication(rx);
  }

  // Generate UCI indication for HARQ bits multiplexed on PUSCH (if present).
  if (pdu.pusch_uci.has_value() && pdu.pusch_uci->harq_ack_bit.value() > 0) {
    const unsigned n_bits = pdu.pusch_uci->harq_ack_bit.value();

    fapi::uci_harq_pdu harq_pdu{};
    harq_pdu.detection_status    = uci_pusch_or_pucch_f2_3_4_detection_status::crc_pass;
    harq_pdu.expected_bit_length = units::bits(n_bits);
    harq_pdu.payload.resize(n_bits);
    harq_pdu.payload.fill(true); // all ACKs

    fapi::uci_pusch_pdu uci_pdu{};
    uci_pdu.handle         = pdu.handle;
    uci_pdu.rnti           = pdu.rnti;
    uci_pdu.ul_sinr_metric = DUMMY_SINR;
    uci_pdu.rssi           = DISABLED_METRIC;
    uci_pdu.rsrp           = DISABLED_METRIC;
    uci_pdu.harq           = harq_pdu;

    fapi::uci_indication ind{};
    ind.slot = slot;
    ind.pdu  = uci_pdu;
    notifier->on_uci_indication(ind);
  }
}

void fapi_dummy_ue_simulator::process_pucch(slot_point slot, const fapi::ul_pucch_pdu& pdu)
{
  if (notifier == nullptr) {
    return;
  }

  // Helper: build a format-0/1 UCI indication with positive HARQ.
  auto make_f0_f1 = [&](fapi::uci_pucch_pdu_format_0_1::format_type fmt,
                         unsigned                                    n_harq) -> fapi::uci_indication {
    fapi::uci_pucch_pdu_format_0_1 uci{};
    uci.handle         = pdu.handle;
    uci.rnti           = pdu.rnti;
    uci.pucch_format   = fmt;
    uci.ul_sinr_metric = DUMMY_SINR;
    uci.rssi           = DISABLED_METRIC;
    uci.rsrp           = DISABLED_METRIC;
    if (n_harq > 0) {
      fapi::uci_harq_format_0_1 harq{};
      for (unsigned i = 0; i < n_harq && i < fapi::uci_harq_format_0_1::MAX_NUM_HARQ; ++i) {
        harq.harq_values.push_back(uci_pucch_f0_or_f1_harq_values::ack);
      }
      uci.harq = harq;
    }
    fapi::uci_indication ind{};
    ind.slot = slot;
    ind.pdu  = uci;
    return ind;
  };

  // Helper: build a format-2/3/4 UCI indication with positive HARQ.
  auto make_f234 = [&](fapi::uci_pucch_pdu_format_2_3_4::format_type fmt,
                        unsigned                                      n_harq) -> fapi::uci_indication {
    fapi::uci_pucch_pdu_format_2_3_4 uci{};
    uci.handle         = pdu.handle;
    uci.rnti           = pdu.rnti;
    uci.pucch_format   = fmt;
    uci.ul_sinr_metric = DUMMY_SINR;
    uci.rssi           = DISABLED_METRIC;
    uci.rsrp           = DISABLED_METRIC;
    if (n_harq > 0) {
      fapi::uci_harq_pdu harq{};
      harq.detection_status    = uci_pusch_or_pucch_f2_3_4_detection_status::crc_pass;
      harq.expected_bit_length = units::bits(n_harq);
      harq.payload.resize(n_harq);
      harq.payload.fill(true);
      uci.harq = harq;
    }
    fapi::uci_indication ind{};
    ind.slot = slot;
    ind.pdu  = uci;
    return ind;
  };

  using F01  = fapi::uci_pucch_pdu_format_0_1::format_type;
  using F234 = fapi::uci_pucch_pdu_format_2_3_4::format_type;

  if (const auto* f0 = std::get_if<fapi::ul_pucch_pdu_format_0>(&pdu.format)) {
    notifier->on_uci_indication(make_f0_f1(F01::format_0, f0->bit_len_harq.value()));
  } else if (const auto* f1 = std::get_if<fapi::ul_pucch_pdu_format_1>(&pdu.format)) {
    notifier->on_uci_indication(make_f0_f1(F01::format_1, f1->bit_len_harq.value()));
  } else if (const auto* f2 = std::get_if<fapi::ul_pucch_pdu_format_2>(&pdu.format)) {
    notifier->on_uci_indication(make_f234(F234::format_2, f2->bit_len_harq.value()));
  } else if (const auto* f3 = std::get_if<fapi::ul_pucch_pdu_format_3>(&pdu.format)) {
    notifier->on_uci_indication(make_f234(F234::format_3, f3->bit_len_harq.value()));
  } else if (const auto* f4 = std::get_if<fapi::ul_pucch_pdu_format_4>(&pdu.format)) {
    notifier->on_uci_indication(make_f234(F234::format_4, f4->bit_len_harq.value()));
  }
  // std::monostate (no format set) — ignore.
}
