// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "fapi_dummy_ue_simulator.h"
#include "ocudu/adt/byte_buffer.h"
#include "ocudu/adt/span.h"
#include "ocudu/ran/phy_time_unit.h"
#include "ocudu/fapi/p7/messages/crc_indication.h"
#include "ocudu/fapi/p7/messages/rach_indication.h"
#include "ocudu/fapi/p7/messages/rx_data_indication.h"
#include "ocudu/fapi/p7/messages/uci_indication.h"
#include "ocudu/fapi/p7/messages/ul_tti_request.h"
#include "ocudu/fapi/p7/p7_indications_notifier.h"
#include "ocudu/ran/uci/uci_mapping.h"
#include "ocudu/ran/uci/uci_payload_type.h"
#include "ocudu/security/security.h"
#include "ocudu/security/security_engine_factory.h"
#include <algorithm>
#include <limits>
#include <vector>

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

// Full 37-byte MAC PDU: [LCID=1 subheader | len=35 | 35B RLC AM PDU with rrcSetupComplete + NAS RegistrationRequest]
// Reused verbatim from mac_test_mode_helpers.cpp — same hardcoded PDU used by DU test mode.
static constexpr std::array<uint8_t, 37> RRC_SETUP_COMPLETE_MAC_PDU = {
    0x01, 0x23, 0xc0, 0x00, 0x00, 0x00, 0x10, 0x00, 0x05, 0xdf, 0x80, 0x10, 0x5e,
    0x40, 0x03, 0x40, 0x40, 0x3c, 0x44, 0x3c, 0x3f, 0xc0, 0x00, 0x04, 0x0c, 0x95,
    0x1d, 0xa6, 0x0b, 0x80, 0xb8, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00};

// SecurityModeComplete PDCP payload (header + RRC, before integrity protection):
//   PDCP: [0x00=R|R|R|R|SN[11:8]=0, 0x01=SN[7:0]=1]  (SRB1, SN=1)
//   RRC:  [0x2A=UL-DCCH c1=smc choice5 txId=1, 0x00=empty IEs]
// NIA2 MAC-I (4 bytes) is appended at runtime by the security engine.
static constexpr std::array<uint8_t, 4> SMC_PDCP_PAYLOAD = {0x00, 0x01, 0x2A, 0x00};

// RRCReconfigurationComplete PDCP payload (header + RRC, before integrity protection):
//   PDCP: [0x00=R|R|R|R|SN[11:8]=0, 0x02=SN[7:0]=2]  (SRB1, SN=2)
//   RRC:  [0x0C=UL-DCCH c1=rrcRecfgComplete choice1 txId=2, 0x00=empty IEs]
// NIA2 MAC-I (4 bytes) is appended at runtime by the security engine (count=2).
static constexpr std::array<uint8_t, 4> RRC_RECONFIG_COMPLETE_PDCP_PAYLOAD = {0x00, 0x02, 0x0C, 0x00};

// SINR representative of a strong signal (~30 dB). Encoded as int16_t at 0.002 dB/LSB.
static constexpr int16_t  DUMMY_SINR = 15000;
// Disabled RSSI/RSRP sentinel per SCF-222.
static constexpr uint16_t DISABLED_METRIC = std::numeric_limits<uint16_t>::max();

// RACH power/SNR encoded per SCF-222: rssi_dBm = (fapi_rssi - 140000) * 0.001
//                                      snr_dB   = (fapi_snr  - 128)    * 0.5
static constexpr uint32_t RACH_RSSI = 130'000U; // -10 dBm
static constexpr uint8_t  RACH_SNR  = 188U;     // 30 dB: (30 / 0.5) + 128

fapi_dummy_ue_simulator::fapi_dummy_ue_simulator(const fapi_dummy_ue_config& cfg_) : cfg(cfg_)
{
  init_rrc_security();
}

void fapi_dummy_ue_simulator::init_rrc_security()
{
  // Mirror the security context the CU-CP will establish for each simulated UE.
  // K_gNB = all-ones (matches the stub's security_key). NIA2 is selected by the gNB
  // preference list given ue_security_cap supports NIA1+NIA2. NEA0 = null cipher.
  security::security_context ctx;
  ctx.k.fill(0xFFU);
  ctx.sel_algos.algos_selected = true;
  ctx.sel_algos.integ_algo     = security::integrity_algorithm::nia2;
  ctx.sel_algos.cipher_algo    = security::ciphering_algorithm::nea0;
  ctx.generate_as_keys();

  security::sec_128_as_config rrc_cfg = ctx.get_128_as_config(security::sec_domain::rrc);

  // SecurityModeComplete is sent with integrity ON but ciphering OFF (NEA0 not yet activated).
  // This matches the CU-CP RX engine state at the time it processes the SecurityModeComplete.
  // SRB1 bearer_id = srb_id_to_uint(srb1) - 1 = 0 (matches pdcp_entity_tx_rx_base.h:66).
  security_engine_creation_message msg{
      .sec_cfg           = rrc_cfg,
      .bearer_id         = 0U,
      .direction         = security::security_direction::uplink,
      .integrity_enabled = security::integrity_enabled::on,
      .ciphering_enabled = security::ciphering_enabled::off,
  };
  rrc_sec_engine = create_security_engine_tx(msg);
}

void fapi_dummy_ue_simulator::store_ul_tti(const fapi::ul_tti_request& msg)
{
  slot_data& entry = buffer[msg.slot.system_slot() % BUFFER_SIZE];
  entry.valid    = true;
  entry.ul_slot  = msg.slot;
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
  // No timer-based state advance needed: registered → rrc_reconfig_pending is triggered
  // by on_dl_srb1_pdu() when the DL rrcReconfiguration bytes are detected.

  maybe_fire_rach(slot);

  slot_data& entry = buffer[slot.system_slot() % BUFFER_SIZE];
  if (!entry.valid) {
    return;
  }

  // Take ownership and clear the buffer entry before firing callbacks.
  slot_data local_data;
  std::swap(local_data, entry);

  for (const auto& pdu : local_data.pusch_pdus) {
    process_pusch(local_data.ul_slot, pdu);
  }
  for (const auto& pdu : local_data.pucch_pdus) {
    process_pucch(local_data.ul_slot, pdu);
  }
}

void fapi_dummy_ue_simulator::flush_ul(slot_point slot)
{
  slot_data& entry = buffer[slot.system_slot() % BUFFER_SIZE];
  if (!entry.valid) {
    return;
  }

  slot_data local_data;
  std::swap(local_data, entry);

  for (const auto& pdu : local_data.pusch_pdus) {
    process_pusch(local_data.ul_slot, pdu);
  }
  for (const auto& pdu : local_data.pucch_pdus) {
    process_pucch(local_data.ul_slot, pdu);
  }
}

void fapi_dummy_ue_simulator::maybe_fire_rach(slot_point slot)
{
  if (notifier == nullptr || cfg.nof_ues == 0 || next_ue_index >= cfg.nof_ues) {
    return;
  }

  // Initialise the first RACH slot to the first slot seen, offset by the per-cell
  // stagger. This prevents cells from firing their DRB setup events simultaneously,
  // which would cause a data race in the shared logical_channel_system::lc_mapper.
  if (next_rach_slot == RACH_SLOT_UNSET) {
    next_rach_slot = slot.system_slot() + cfg.rach_start_offset_slots;
  }

  if (slot.system_slot() < next_rach_slot) {
    return;
  }

  fapi::rach_indication_pdu_preamble preamble{};
  preamble.preamble_index        = static_cast<uint8_t>(next_ue_index % 64U);
  preamble.timing_advance_offset = phy_time_unit::from_units_of_Tc(0);
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

    // Build a MAC UL PDU whose content depends on the UE's current attach state.
    //
    // State machine per C-RNTI (first seen → ccch_pending):
    //   ccch_pending  → Msg3 (RRCSetupRequest on CCCH, LCID=0); advance to srb1_pending.
    //   srb1_pending  → rrcSetupComplete on SRB1 (LCID=1) if TBS is large enough;
    //                   otherwise padding (retry on next grant).
    //   srb1_sent     → padding; waiting for DL PDSCH (SecurityModeCommand signal).
    //   smc_pending   → SecurityModeComplete on SRB1 (LCID=1); advance to registered.
    //   registered    → padding only.
    const uint32_t tbs = data.tb_size.value();
    std::vector<uint8_t> tb_pdu(tbs, 0U);

    auto& state = rnti_states[pdu.rnti]; // default-constructs to ccch_pending (value 0)
    switch (state) {
      case ue_ul_state::ccch_pending:
        // Msg3: RRCSetupRequest on CCCH (LCID=0, fixed 8-byte payload) followed by a
        // Short BSR CE (2 bytes) to signal pending SRB1 data to the scheduler.
        // Without a BSR the scheduler has no reason to grant another UL PUSCH after
        // Msg3, so rrcSetupComplete could never be sent.
        if (tbs >= 9) {
          tb_pdu[0] = 0x00U; // CCCH_SIZE_64 subheader (LCID=0, 8-byte fixed payload)
          // bytes 1-8: zero CCCH payload (RRCSetupRequest placeholder)
          if (tbs >= 11) {
            // Short BSR: LCG=0, BufferSizeLevel=31 (~350 bytes) — TS 38.321 §6.1.3.1.
            tb_pdu[9]  = 0x3DU; // Short BSR subheader (LCID=0x3D)
            tb_pdu[10] = 0x1FU; // LCG=0 (bits 7-5), level=31 (bits 4-0)
            if (tbs > 11) {
              tb_pdu[11] = 0x3FU; // PADDING LCID — consumes remainder
            }
          } else if (tbs == 10) {
            tb_pdu[9] = 0x3FU; // 1 byte left — only padding fits
          }
        } else {
          tb_pdu[0] = 0x3FU; // too small for CCCH — padding only
        }
        state = ue_ul_state::srb1_pending;
        break;
      case ue_ul_state::srb1_pending:
        if (tbs >= RRC_SETUP_COMPLETE_MAC_PDU.size()) {
          std::copy(RRC_SETUP_COMPLETE_MAC_PDU.begin(), RRC_SETUP_COMPLETE_MAC_PDU.end(), tb_pdu.begin());
          if (tbs > RRC_SETUP_COMPLETE_MAC_PDU.size()) {
            tb_pdu[RRC_SETUP_COMPLETE_MAC_PDU.size()] = 0x3FU; // trailing PADDING LCID
          }
          state = ue_ul_state::srb1_sent;
        } else {
          // TBS too small to carry rrcSetupComplete — send padding and retry next grant.
          tb_pdu[0] = 0x3FU;
        }
        break;
      case ue_ul_state::srb1_sent:
        tb_pdu[0] = 0x3FU; // waiting for DL SecurityModeCommand detection
        break;
      case ue_ul_state::smc_pending: {
        // MAC TB: 2B MAC subhdr + 2B RLC AM hdr + 4B PDCP payload + 4B NIA2 MAC-I = 12 bytes.
        static constexpr unsigned SMC_MAC_PDU_SIZE = 12U;
        // Do not send SecurityModeComplete until the DL SecurityModeCommand has been
        // parsed and its txId stored — smc_txid is populated by on_dl_srb1_pdu when the
        // SMC MAC TB arrives.  If the pre-SMC DL PDSCH triggered the srb1_sent→smc_pending
        // transition before the SMC slot, send padding and wait for the next grant.
        auto it_txid = smc_txid.find(pdu.rnti);
        if (it_txid == smc_txid.end()) {
          tb_pdu[0] = 0x3FU; // padding — SMC not received yet
          break;
        }
        if (rrc_sec_engine != nullptr && tbs >= SMC_MAC_PDU_SIZE) {
          // Patch the RRC byte with the transaction ID extracted from the DL SecurityModeCommand.
          uint8_t txid = it_txid->second;
          std::array<uint8_t, 4> smc_payload = SMC_PDCP_PAYLOAD;
          smc_payload[2] = static_cast<uint8_t>(0x28U | (txid << 1U));
          byte_buffer pdcp_buf;
          (void)pdcp_buf.append(span<const uint8_t>(smc_payload.data(), smc_payload.size()));
          auto sec_result = rrc_sec_engine->encrypt_and_protect_integrity(std::move(pdcp_buf), 2U, 1U);
          if (sec_result.buf.has_value()) {
            tb_pdu[0] = 0x01U; // MAC: LCID=1
            tb_pdu[1] = 0x0AU; // MAC: len=10 (2 RLC + 8 PDCP+MAC-I)
            tb_pdu[2] = 0x80U; // RLC AM: D/C=1, P=0, SI=00, SN[11:8]=0
            tb_pdu[3] = 0x01U; // RLC AM: SN[7:0]=1
            unsigned off = 4U;
            for (uint8_t byte : sec_result.buf.value()) {
              tb_pdu[off++] = byte;
            }
            if (tbs > SMC_MAC_PDU_SIZE) {
              tb_pdu[SMC_MAC_PDU_SIZE] = 0x3FU;
            }
            state = ue_ul_state::registered;
          } else {
            tb_pdu[0] = 0x3FU;
          }
        } else {
          tb_pdu[0] = 0x3FU;
        }
        break;
      }
      case ue_ul_state::registered: {
        // Send a STATUS PDU for SRB1 DL (ACK_SN=1) on every PUSCH while waiting for
        // the gNB to send RRCReconfiguration.  Without this, the gNB's RLC AM layer
        // exhausts retransmissions on the SMC poll before our rrc_reconfig_pending
        // STATUS PDU (ACK_SN=2) is ever sent.
        static constexpr unsigned SRB1_STATUS_SIZE = 5U; // 2B MAC subhdr + 3B RLC STATUS
        if (tbs >= SRB1_STATUS_SIZE) {
          tb_pdu[0] = 0x01U; // MAC: LCID=1, F=0
          tb_pdu[1] = 0x03U; // MAC: L=3
          tb_pdu[2] = 0x00U; // RLC STATUS: D/C=0, CPT=000, ACK_SN[11:8]=0
          tb_pdu[3] = 0x01U; // RLC STATUS: ACK_SN[7:0]=1  (ACKs SN=0 = SMC)
          tb_pdu[4] = 0x00U; // RLC STATUS: E1=0
          if (tbs > SRB1_STATUS_SIZE) {
            tb_pdu[SRB1_STATUS_SIZE] = 0x3FU; // padding
          }
        } else {
          tb_pdu[0] = 0x3FU;
        }
        break;
      }
      case ue_ul_state::rrc_reconfig_pending: {
        // STATUS PDU (5B, ACK_SN=2) + RRCReconfigurationComplete (12B, NIA2 count=2) = 17B.
        // SRB1 DL SNs: 0=SMC, 1=rrcReconfiguration (no DLInfoTransfer in no_core mode).
        // ACK_SN=2 pre-ACKs SN=1 (rrcReconfiguration) even before RLC assigns it a SN.
        // The protocol-failure check is ack_sn > tx_next+1; with tx_next=1 (SMC sent),
        // ACK_SN=2 ≤ tx_next+1=2 → safe. Pre-ACK prevents post-ICS SRB1 RLC max-retx.
        static constexpr unsigned RECONFIG_MAC_PDU_SIZE = 17U;
        if (rrc_sec_engine != nullptr && tbs >= RECONFIG_MAC_PDU_SIZE) {
          tb_pdu[0] = 0x01U; // MAC: LCID=1
          tb_pdu[1] = 0x03U; // MAC: length=3
          tb_pdu[2] = 0x00U; // RLC STATUS: D/C=0, CPT=000, ACK_SN[11:8]=0
          tb_pdu[3] = 0x02U; // RLC STATUS: ACK_SN[7:0]=2  (pre-ACKs SNs 0-1)
          tb_pdu[4] = 0x00U; // RLC STATUS: E1=0
          // Patch the RRC byte with the transaction ID extracted from the DL rrcReconfiguration.
          auto it_txid = reconfig_txid.find(pdu.rnti);
          uint8_t rtxid = (it_txid != reconfig_txid.end()) ? it_txid->second : 2U;
          std::array<uint8_t, 4> reconfig_payload = RRC_RECONFIG_COMPLETE_PDCP_PAYLOAD;
          reconfig_payload[2] = static_cast<uint8_t>(0x08U | (rtxid << 1U)); // UL rrcReconfigComplete: c1_idx=1 → 0x08|(txId<<1)
          byte_buffer pdcp_buf;
          (void)pdcp_buf.append(
              span<const uint8_t>(reconfig_payload.data(), reconfig_payload.size()));
          auto sec_result = rrc_sec_engine->encrypt_and_protect_integrity(std::move(pdcp_buf), 2U, 2U);
          if (sec_result.buf.has_value()) {
            tb_pdu[5] = 0x01U; // MAC: LCID=1
            tb_pdu[6] = 0x0AU; // MAC: length=10 (2 RLC + 8 PDCP+MAC-I)
            tb_pdu[7] = 0x80U; // RLC AM: D/C=1, P=0, SI=00, SN[11:8]=0
            tb_pdu[8] = 0x02U; // RLC AM: SN[7:0]=2
            unsigned off = 9U;
            for (uint8_t byte : sec_result.buf.value()) {
              tb_pdu[off++] = byte;
            }
            if (tbs > RECONFIG_MAC_PDU_SIZE) {
              tb_pdu[RECONFIG_MAC_PDU_SIZE] = 0x3FU;
            }
            state = ue_ul_state::drb_active;
          } else {
            tb_pdu[0] = 0x3FU;
          }
        } else {
          tb_pdu[0] = 0x3FU;
        }
        break;
      }
      case ue_ul_state::drb_active: {
        // Sustained UL data on DRB (LCID=4): Long BSR + RLC AM + PDCP + zero payload.
        // Long BSR: all 8 LCGs at max buffer level to keep scheduler granting UL.
        // RLC AM and PDCP DRB both use 18-bit SN (3-byte headers each, NEA0 = no cipher).
        // MAC subheader uses F=1 (2-byte L field) to support any TBS size.
        static constexpr unsigned LONG_BSR_SIZE = 11U; // 1B subhdr + 1B len + 1B bitmap + 8B levels
        static constexpr unsigned MAC_SUBHDR_SIZE = 3U; // F=1: LCID byte + 2-byte L field
        static constexpr unsigned RLC_HDR_SIZE  = 3U;  // 18-bit AM SN header
        static constexpr unsigned PDCP_HDR_SIZE = 3U;  // 18-bit DRB SN header
        static constexpr unsigned DRB_MIN_TB = LONG_BSR_SIZE + MAC_SUBHDR_SIZE + RLC_HDR_SIZE + PDCP_HDR_SIZE;
        if (tbs >= DRB_MIN_TB) {
          // Long BSR: LCID=0x3E, F=0, len=9 (1B bitmap + 8B levels for all LCGs)
          tb_pdu[0]  = 0x3EU; // MAC: Long BSR subheader (LCID=0x3E)
          tb_pdu[1]  = 0x09U; // MAC: BSR payload length=9
          tb_pdu[2]  = 0xFFU; // LCG bitmap: LCGs 7-0 all present
          tb_pdu[3]  = 0xFEU; // LCG 0: max valid level (255 is reserved/invalid per TS 38.321)
          tb_pdu[4]  = 0xFEU; // LCG 1
          tb_pdu[5]  = 0xFEU; // LCG 2
          tb_pdu[6]  = 0xFEU; // LCG 3
          tb_pdu[7]  = 0xFEU; // LCG 4
          tb_pdu[8]  = 0xFEU; // LCG 5
          tb_pdu[9]  = 0xFEU; // LCG 6
          tb_pdu[10] = 0xFEU; // LCG 7
          uint32_t sn  = drb_ul_sn[pdu.rnti]++ & 0x3FFFFU; // 18-bit SN
          unsigned rlc_pdu_len = tbs - LONG_BSR_SIZE - MAC_SUBHDR_SIZE; // RLC PDU = everything after BSR+subhdr
          tb_pdu[11] = 0x44U;                                              // MAC: R=0, F=1, LCID=4
          tb_pdu[12] = static_cast<uint8_t>((rlc_pdu_len >> 8U) & 0x7FU); // MAC: L[14:8]
          tb_pdu[13] = static_cast<uint8_t>(rlc_pdu_len & 0xFFU);          // MAC: L[7:0]
          tb_pdu[14] = static_cast<uint8_t>(0x80U | ((sn >> 16U) & 0x03U)); // RLC AM: D/C=1,P=0,SI=00,SN[17:16]
          tb_pdu[15] = static_cast<uint8_t>((sn >> 8U) & 0xFFU);            // RLC AM: SN[15:8]
          tb_pdu[16] = static_cast<uint8_t>(sn & 0xFFU);                    // RLC AM: SN[7:0]
          tb_pdu[17] = static_cast<uint8_t>(0x80U | ((sn >> 16U) & 0x03U)); // PDCP: D/C=1,R(5)=0,SN[17:16]
          tb_pdu[18] = static_cast<uint8_t>((sn >> 8U) & 0xFFU);            // PDCP: SN[15:8]
          tb_pdu[19] = static_cast<uint8_t>(sn & 0xFFU);                    // PDCP: SN[7:0]
          // Payload bytes 20..tbs-1 remain zero (NEA0 null cipher = plaintext zeros).
        } else {
          tb_pdu[0] = 0x3FU;
        }
        break;
      }
    }

    fapi::rx_data_indication rx{};
    rx.slot                = slot;
    rx.pdu.handle          = pdu.handle;
    rx.pdu.rnti            = pdu.rnti;
    rx.pdu.harq_id         = data.harq_process_id;
    rx.pdu.transport_block = span<const uint8_t>(tb_pdu.data(), tb_pdu.size());
    notifier->on_rx_data_indication(rx);
  }

  // Generate UCI indication for HARQ/CSI bits multiplexed on PUSCH (if present).
  if (pdu.pusch_uci.has_value()) {
    const unsigned n_harq = pdu.pusch_uci->harq_ack_bit.value();
    const unsigned n_csi  = pdu.pusch_uci->csi_part1_bit.value();
    if (n_harq > 0 || n_csi > 0) {
      fapi::uci_pusch_pdu uci_pdu{};
      uci_pdu.handle         = pdu.handle;
      uci_pdu.rnti           = pdu.rnti;
      uci_pdu.ul_sinr_metric = DUMMY_SINR;
      uci_pdu.rssi           = DISABLED_METRIC;
      uci_pdu.rsrp           = DISABLED_METRIC;
      if (n_harq > 0) {
        fapi::uci_harq_pdu harq_pdu{};
        harq_pdu.detection_status    = uci_pusch_or_pucch_f2_3_4_detection_status::crc_pass;
        harq_pdu.expected_bit_length = units::bits(n_harq);
        harq_pdu.payload.resize(n_harq);
        harq_pdu.payload.fill(true); // all ACKs
        uci_pdu.harq = harq_pdu;
      }
      if (n_csi > 0) {
        fapi::uci_csi_part1 csi{};
        csi.detection_status    = uci_pusch_or_pucch_f2_3_4_detection_status::crc_pass;
        csi.expected_bit_length = units::bits(n_csi);
        csi.payload.resize(n_csi);
        csi.payload.fill(true); // all-ones: CQI=15 (last 4 bits)
        uci_pdu.csi_part1 = csi;
      }
      fapi::uci_indication ind{};
      ind.slot = slot;
      ind.pdu  = uci_pdu;
      notifier->on_uci_indication(ind);
    }
  }
}

void fapi_dummy_ue_simulator::process_pucch(slot_point slot, const fapi::ul_pucch_pdu& pdu)
{
  if (notifier == nullptr) {
    return;
  }

  // Helper: build a format-0/1 UCI indication with positive HARQ and optional SR.
  // sr_bits > 0 means the scheduler allocated SR resources in this PUCCH.
  // We signal SR=detected when the UE is waiting to send rrcSetupComplete (srb1_pending),
  // which causes the scheduler to grant a follow-up PUSCH for the rrcSetupComplete PDU.
  auto make_f0_f1 = [&](fapi::uci_pucch_pdu_format_0_1::format_type fmt,
                         unsigned                                    n_harq,
                         bool                                        sr_present) -> fapi::uci_indication {
    fapi::uci_pucch_pdu_format_0_1 uci{};
    uci.handle         = pdu.handle;
    uci.rnti           = pdu.rnti;
    uci.pucch_format   = fmt;
    uci.ul_sinr_metric = DUMMY_SINR;
    uci.rssi           = DISABLED_METRIC;
    uci.rsrp           = DISABLED_METRIC;
    if (sr_present) {
      auto it = rnti_states.find(pdu.rnti);
      bool pending = it != rnti_states.end() &&
                     (it->second == ue_ul_state::srb1_pending ||
                      it->second == ue_ul_state::smc_pending   ||
                      it->second == ue_ul_state::registered    ||
                      it->second == ue_ul_state::rrc_reconfig_pending ||
                      it->second == ue_ul_state::drb_active);
      uci.sr = fapi::sr_pdu_format_0_1{.sr_detected = pending};
    }
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

  // Helper: build a format-2/3/4 UCI indication with positive HARQ and CQI=15 CSI.
  // n_csi is the csi_part1_bit_length from the UL_TTI_request PDU. For 1T1R wideband
  // CQI the scheduler allocates exactly 4 bits; setting all bits to 1 encodes CQI=15.
  // For multi-port configs the leading RI/PMI bits are also set to max, which is safe
  // since the scheduler clamps RI to nof_dl_ports anyway.
  auto make_f234 = [&](fapi::uci_pucch_pdu_format_2_3_4::format_type fmt,
                        unsigned                                      n_harq,
                        unsigned                                      n_csi) -> fapi::uci_indication {
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
    if (n_csi > 0) {
      fapi::uci_csi_part1 csi{};
      csi.detection_status    = uci_pusch_or_pucch_f2_3_4_detection_status::crc_pass;
      csi.expected_bit_length = units::bits(n_csi);
      csi.payload.resize(n_csi);
      csi.payload.fill(true); // all-ones: CQI=15 (last 4 bits), RI/PMI bits max
      uci.csi_part1 = csi;
    }
    fapi::uci_indication ind{};
    ind.slot = slot;
    ind.pdu  = uci;
    return ind;
  };

  using F01  = fapi::uci_pucch_pdu_format_0_1::format_type;
  using F234 = fapi::uci_pucch_pdu_format_2_3_4::format_type;

  if (const auto* f0 = std::get_if<fapi::ul_pucch_pdu_format_0>(&pdu.format)) {
    notifier->on_uci_indication(make_f0_f1(F01::format_0, f0->bit_len_harq.value(), f0->sr_present));
  } else if (const auto* f1 = std::get_if<fapi::ul_pucch_pdu_format_1>(&pdu.format)) {
    notifier->on_uci_indication(make_f0_f1(F01::format_1, f1->bit_len_harq.value(), f1->sr_present));
  } else if (const auto* f2 = std::get_if<fapi::ul_pucch_pdu_format_2>(&pdu.format)) {
    notifier->on_uci_indication(make_f234(F234::format_2, f2->bit_len_harq.value(), f2->csi_part1_bit_length.value()));
  } else if (const auto* f3 = std::get_if<fapi::ul_pucch_pdu_format_3>(&pdu.format)) {
    notifier->on_uci_indication(make_f234(F234::format_3, f3->bit_len_harq.value(), f3->csi_part1_bit_length.value()));
  } else if (const auto* f4 = std::get_if<fapi::ul_pucch_pdu_format_4>(&pdu.format)) {
    notifier->on_uci_indication(make_f234(F234::format_4, f4->bit_len_harq.value(), f4->csi_part1_bit_length.value()));
  }
  // std::monostate (no format set) — ignore.
}

void fapi_dummy_ue_simulator::on_dl_pdsch_for_rnti(rnti_t rnti)
{
  auto it = rnti_states.find(rnti);
  if (it == rnti_states.end()) {
    return;
  }
  if (it->second == ue_ul_state::srb1_sent) {
    it->second = ue_ul_state::smc_pending;
  // registered state: transition is handled by the slot timer in on_new_slot.
  }
}

void fapi_dummy_ue_simulator::on_dl_srb1_pdu(rnti_t rnti, const uint8_t* data, size_t len)
{
  // Scan the MAC TB for an SRB1 (LCID=1) SDU and extract the RRC transaction ID
  // from the first byte of the RRC message (bits [2:1]).
  //
  // MAC PDU layout: [R|F|LCID][length][RLC AM hdr (2B)][PDCP hdr (2B)][RRC byte 0][...]
  // Ciphering is NEA0 (null) so the RRC payload is plaintext.
  //
  // DL-DCCH first byte:
  //   bit7=0 (c1), bits6-3=c1_choice_index (4 bits), bits2-1=txId (2 bits), bit0=critExt
  //   SecurityModeCommand:  c1_idx=4 → byte & 0xF9 == 0x20; txId=(byte>>1)&3
  //   rrcReconfiguration:   c1_idx=0 → byte & 0xF9 == 0x00; txId=(byte>>1)&3

  size_t off = 0;
  while (off < len) {
    uint8_t hdr  = data[off];
    uint8_t lcid = hdr & 0x3FU;
    bool    F    = (hdr & 0x40U) != 0;
    off++;

    if (lcid == 63U) { // padding
      break;
    }

    // Fixed-size MAC CEs in DL have no length field — skip them by known sizes.
    // Variable-size entries (LCIDs 0-25) always have a length field.
    static constexpr uint8_t FIXED_CE_LCID_MIN = 26U;
    if (lcid >= FIXED_CE_LCID_MIN) {
      // Skip fixed-size CEs: we don't need them for txId extraction.
      break;
    }

    size_t sdu_len;
    if (F) {
      if (off + 2 > len) {
        break;
      }
      sdu_len = (static_cast<size_t>(data[off]) << 8U) | data[off + 1];
      off += 2;
    } else {
      if (off + 1 > len) {
        break;
      }
      sdu_len = data[off];
      off++;
    }

    if (lcid == 1U && sdu_len >= 5U && off + sdu_len <= len) {
      // SDU is an RLC AM PDU. Check it's not an RLC control PDU or a mid/last segment.
      // RLC AM header byte 0: D/C (bit7), P (bit6), SI[1:0] (bits5-4), SN[11:8] (bits3-0).
      uint8_t rlc_hdr = data[off];
      bool    rlc_dc  = (rlc_hdr & 0x80U) != 0; // 1=data, 0=control
      uint8_t rlc_si  = (rlc_hdr >> 4U) & 0x03U; // 00=full, 01=first, 10=middle, 11=last
      if (rlc_dc && (rlc_si == 0x00U || rlc_si == 0x01U)) {
        // Full or first segment: PDCP header follows at offset 2; RRC byte at offset 4.
        // 12-bit PDCP SN: bytes 2-3 of the SDU are the PDCP header (SN[11:8], SN[7:0]).
        uint8_t pdcp_sn_lo = data[off + 3U]; // SN[7:0] (SN < 256 for first few messages)
        size_t  rrc_off    = off + 4U;
        if (rrc_off < off + sdu_len) {
          uint8_t rrc_byte = data[rrc_off];
          uint8_t txid     = static_cast<uint8_t>((rrc_byte >> 1U) & 0x3U);
          if (pdcp_sn_lo <= 2U && (rrc_byte & 0xF9U) == 0x20U) { // SN=0..2: SecurityModeCommand (c1 idx=4)
            smc_txid[rnti] = txid;
          } else if (pdcp_sn_lo <= 3U && (rrc_byte & 0xF9U) == 0x00U) { // SN=1..3: rrcReconfiguration
            reconfig_txid[rnti] = txid;
            // Trigger registered → rrc_reconfig_pending so we respond with the correct txId.
            auto s_it = rnti_states.find(rnti);
            if (s_it != rnti_states.end() && s_it->second == ue_ul_state::registered) {
              s_it->second = ue_ul_state::rrc_reconfig_pending;
            }
          }
        }
      }
      break;
    }

    off += sdu_len;
  }
}
