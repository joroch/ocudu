// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

/// \file
/// Unit tests for the per-RNTI SRB1 state machine in fapi_dummy_ue_simulator.
///
/// Verifies the three-phase attach transition:
///   1. First PUSCH grant → Msg3 CCCH (LCID=0, RRCSetupRequest placeholder)
///   2. Next sufficient PUSCH grant → rrcSetupComplete on SRB1 (LCID=1)
///   3. All subsequent PUSCH grants → PADDING (LCID=0x3F)

#include "lib/fapi_adaptor/dummy/fapi_dummy_ue_simulator.h"
#include "ocudu/fapi/p7/messages/crc_indication.h"
#include "ocudu/fapi/p7/messages/rx_data_indication.h"
#include "ocudu/fapi/p7/messages/uci_indication.h"
#include "ocudu/fapi/p7/messages/ul_tti_request.h"
#include "ocudu/fapi/p7/p7_indications_notifier.h"
#include "ocudu/ran/harq_id.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/support/units.h"
#include <gtest/gtest.h>
#include <vector>

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

namespace {

// Expected rrcSetupComplete MAC PDU prefix (LCID=1 subheader + length byte).
static constexpr uint8_t SRB1_LCID        = 0x01U;
static constexpr uint8_t SRB1_PDU_LEN     = 0x23U; // 35 bytes of RLC payload
static constexpr size_t  SRB1_PDU_TOTAL   = 37U;   // 2-byte subheader + 35-byte RLC body
static constexpr uint8_t CCCH_SUBHDR       = 0x00U; // CCCH_SIZE_64
static constexpr uint8_t PADDING_LCID      = 0x3FU;
static constexpr uint8_t SHORT_BSR_LCID    = 0x3DU; // Short BSR subheader (TS 38.321 §6.1.3.1)
static constexpr uint8_t SHORT_BSR_PAYLOAD = 0x1FU; // LCG=0, BufferSizeLevel=31 (~350 bytes)

/// Spy notifier that copies transport-block bytes synchronously before the local vector
/// in process_pusch() is destroyed.
class indications_spy : public fapi::p7_indications_notifier
{
public:
  struct rx_entry {
    rnti_t               rnti;
    std::vector<uint8_t> tb; // deep copy of the transport block
  };

  void on_rx_data_indication(const fapi::rx_data_indication& msg) override
  {
    rx_entries.push_back({msg.pdu.rnti,
                          std::vector<uint8_t>(msg.pdu.transport_block.begin(),
                                               msg.pdu.transport_block.end())});
  }
  void on_crc_indication(const fapi::crc_indication&) override {}
  void on_uci_indication(const fapi::uci_indication&) override {}
  void on_srs_indication(const fapi::srs_indication&) override {}
  void on_rach_indication(const fapi::rach_indication&) override {}

  std::vector<rx_entry> rx_entries;
};

/// Build a single-PDU UL_TTI.request for \p rnti at \p slot with the given TBS.
fapi::ul_tti_request make_pusch(slot_point slot, rnti_t rnti, units::bytes tbs, harq_id_t harq = to_harq_id(0))
{
  fapi::ul_pusch_pdu pdu{};
  pdu.rnti   = rnti;
  pdu.handle = 1U;

  fapi::ul_pusch_data data{};
  data.harq_process_id = harq;
  data.tb_size         = tbs;
  data.rv_index        = 0;
  data.new_data        = true;
  pdu.pusch_data       = data;

  fapi::ul_tti_request_pdu wrapper{};
  wrapper.pdu = pdu;

  fapi::ul_tti_request req{};
  req.slot = slot;
  req.pdus.push_back(wrapper);
  return req;
}

/// Helper: drive one PUSCH grant for \p rnti and return the received TB bytes.
std::vector<uint8_t>
drive_one_pusch(fapi_dummy_ue_simulator& sim, indications_spy& spy, slot_point slot, rnti_t rnti, unsigned tbs_bytes)
{
  const size_t before = spy.rx_entries.size();
  sim.store_ul_tti(make_pusch(slot, rnti, units::bytes(tbs_bytes)));
  sim.on_new_slot(slot);
  if (spy.rx_entries.size() == before) {
    return {}; // no indication fired
  }
  return spy.rx_entries.back().tb;
}

} // namespace

class fapi_dummy_srb1_test : public ::testing::Test
{
protected:
  fapi_dummy_srb1_test()
  {
    sim.set_notifier(spy);
  }

  indications_spy         spy;
  fapi_dummy_ue_simulator sim;

  static constexpr rnti_t UE1 = to_rnti(0xC001);
  static constexpr rnti_t UE2 = to_rnti(0xC002);

  slot_point slot(unsigned s) const { return {subcarrier_spacing::kHz15, s}; }
};

// ---------------------------------------------------------------------------
// CCCH phase
// ---------------------------------------------------------------------------

TEST_F(fapi_dummy_srb1_test, first_pusch_sends_ccch_msg3)
{
  auto tb = drive_one_pusch(sim, spy, slot(0), UE1, 100);

  ASSERT_EQ(tb.size(), 100U);
  EXPECT_EQ(tb[0], CCCH_SUBHDR);
  // bytes 1-8: CCCH payload (zeros)
  // bytes 9-10: Short BSR CE signals pending SRB1 data to scheduler
  EXPECT_EQ(tb[9],  SHORT_BSR_LCID);
  EXPECT_EQ(tb[10], SHORT_BSR_PAYLOAD);
  EXPECT_EQ(tb[11], PADDING_LCID);
}

TEST_F(fapi_dummy_srb1_test, first_pusch_small_tbs_sends_padding_but_advances_state)
{
  // TBS < 9: CCCH doesn't fit, send padding. State still advances to srb1_pending.
  auto tb1 = drive_one_pusch(sim, spy, slot(0), UE1, 5);
  EXPECT_EQ(tb1[0], PADDING_LCID);

  // Next sufficient grant must deliver SRB1 (not CCCH again).
  auto tb2 = drive_one_pusch(sim, spy, slot(1), UE1, 100);
  ASSERT_GE(tb2.size(), SRB1_PDU_TOTAL);
  EXPECT_EQ(tb2[0], SRB1_LCID);
  EXPECT_EQ(tb2[1], SRB1_PDU_LEN);
}

// ---------------------------------------------------------------------------
// SRB1 phase
// ---------------------------------------------------------------------------

TEST_F(fapi_dummy_srb1_test, second_pusch_sends_rrc_setup_complete_on_srb1)
{
  drive_one_pusch(sim, spy, slot(0), UE1, 100); // Msg3 CCCH

  auto tb = drive_one_pusch(sim, spy, slot(1), UE1, 100);

  ASSERT_GE(tb.size(), SRB1_PDU_TOTAL);
  EXPECT_EQ(tb[0], SRB1_LCID)    << "MAC subheader must indicate LCID=1 (SRB1)";
  EXPECT_EQ(tb[1], SRB1_PDU_LEN) << "Length field must be 35 (0x23)";
  // Trailing space after the 37-byte PDU must be filled with a PADDING LCID.
  EXPECT_EQ(tb[SRB1_PDU_TOTAL], PADDING_LCID);
}

TEST_F(fapi_dummy_srb1_test, srb1_content_matches_known_rrc_setup_complete_bytes)
{
  // Known-good bytes from mac_test_mode_helpers.cpp.
  static constexpr std::array<uint8_t, SRB1_PDU_TOTAL> EXPECTED = {
      0x01, 0x23, 0xc0, 0x00, 0x00, 0x00, 0x10, 0x00, 0x05, 0xdf, 0x80, 0x10, 0x5e,
      0x40, 0x03, 0x40, 0x40, 0x3c, 0x44, 0x3c, 0x3f, 0xc0, 0x00, 0x04, 0x0c, 0x95,
      0x1d, 0xa6, 0x0b, 0x80, 0xb8, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00};

  drive_one_pusch(sim, spy, slot(0), UE1, 100); // Msg3 CCCH
  auto tb = drive_one_pusch(sim, spy, slot(1), UE1, 100);

  ASSERT_GE(tb.size(), SRB1_PDU_TOTAL);
  for (size_t i = 0; i < SRB1_PDU_TOTAL; ++i) {
    EXPECT_EQ(tb[i], EXPECTED[i]) << "mismatch at byte " << i;
  }
}

TEST_F(fapi_dummy_srb1_test, srb1_pending_with_tbs_too_small_retries_next_grant)
{
  drive_one_pusch(sim, spy, slot(0), UE1, 100); // CCCH → srb1_pending

  // Grant too small to carry rrcSetupComplete (37 bytes).
  auto tb_small = drive_one_pusch(sim, spy, slot(1), UE1, 20);
  EXPECT_EQ(tb_small[0], PADDING_LCID) << "TBS < 37 must produce padding, not a truncated SRB1 PDU";

  // Next grant with sufficient TBS must still deliver SRB1 (state stayed srb1_pending).
  auto tb_ok = drive_one_pusch(sim, spy, slot(2), UE1, 100);
  ASSERT_GE(tb_ok.size(), SRB1_PDU_TOTAL);
  EXPECT_EQ(tb_ok[0], SRB1_LCID);
  EXPECT_EQ(tb_ok[1], SRB1_PDU_LEN);
}

// ---------------------------------------------------------------------------
// Post-SRB1 phase (padding only)
// ---------------------------------------------------------------------------

TEST_F(fapi_dummy_srb1_test, third_pusch_sends_padding_after_srb1_sent)
{
  drive_one_pusch(sim, spy, slot(0), UE1, 100); // CCCH
  drive_one_pusch(sim, spy, slot(1), UE1, 100); // SRB1

  auto tb = drive_one_pusch(sim, spy, slot(2), UE1, 100);
  EXPECT_EQ(tb[0], PADDING_LCID) << "All grants after rrcSetupComplete must produce padding";
}

// ---------------------------------------------------------------------------
// Independence of per-RNTI state
// ---------------------------------------------------------------------------

TEST_F(fapi_dummy_srb1_test, different_rntis_have_independent_states)
{
  // UE1 sends Msg3.
  auto tb1_ccch = drive_one_pusch(sim, spy, slot(0), UE1, 100);
  EXPECT_EQ(tb1_ccch[0], CCCH_SUBHDR) << "UE1 first grant must be CCCH";

  // UE2 first grant must also be CCCH regardless of UE1's state.
  auto tb2_ccch = drive_one_pusch(sim, spy, slot(1), UE2, 100);
  EXPECT_EQ(tb2_ccch[0], CCCH_SUBHDR) << "UE2 first grant must be CCCH";

  // UE1 second grant → SRB1.
  auto tb1_srb1 = drive_one_pusch(sim, spy, slot(2), UE1, 100);
  EXPECT_EQ(tb1_srb1[0], SRB1_LCID) << "UE1 second grant must be SRB1";

  // UE2 second grant → SRB1 (independent of UE1).
  auto tb2_srb1 = drive_one_pusch(sim, spy, slot(3), UE2, 100);
  EXPECT_EQ(tb2_srb1[0], SRB1_LCID) << "UE2 second grant must be SRB1";
}

TEST_F(fapi_dummy_srb1_test, ue1_srb1_does_not_affect_ue2_ccch_phase)
{
  // Drive UE1 all the way through to srb1_sent.
  drive_one_pusch(sim, spy, slot(0), UE1, 100); // CCCH
  drive_one_pusch(sim, spy, slot(1), UE1, 100); // SRB1 → srb1_sent
  drive_one_pusch(sim, spy, slot(2), UE1, 100); // padding

  // UE2 arrives late — must still start from CCCH, not inherit UE1's padding state.
  auto tb2 = drive_one_pusch(sim, spy, slot(3), UE2, 100);
  EXPECT_EQ(tb2[0], CCCH_SUBHDR) << "UE2 first grant must be CCCH regardless of UE1's state";
}
