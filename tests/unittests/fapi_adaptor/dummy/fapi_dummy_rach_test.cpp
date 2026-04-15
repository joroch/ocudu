// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "lib/fapi_adaptor/dummy/fapi_dummy_ue_simulator.h"
#include "ocudu/fapi/p7/messages/rach_indication.h"
#include "ocudu/fapi/p7/p7_indications_notifier.h"
#include "ocudu/ran/slot_point.h"
#include <gtest/gtest.h>
#include <vector>

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

namespace {

class indications_spy : public fapi::p7_indications_notifier
{
public:
  void on_rx_data_indication(const fapi::rx_data_indication&) override {}
  void on_crc_indication(const fapi::crc_indication&) override {}
  void on_uci_indication(const fapi::uci_indication&) override {}
  void on_srs_indication(const fapi::srs_indication&) override {}
  void on_rach_indication(const fapi::rach_indication& msg) override { rachs.push_back(msg); }

  std::vector<fapi::rach_indication> rachs;
};

/// Advance the simulator by one slot and return its slot_point.
slot_point advance(fapi_dummy_ue_simulator& sim, unsigned slot_count)
{
  slot_point slot{subcarrier_spacing::kHz15, slot_count};
  sim.on_new_slot(slot);
  return slot;
}

} // namespace

TEST(fapi_dummy_rach_test, rach_fires_at_staggered_slots)
{
  fapi_dummy_ue_config cfg;
  cfg.nof_ues                   = 3;
  cfg.ue_creation_stagger_slots = 10;

  fapi_dummy_ue_simulator sim(cfg);
  indications_spy         spy;
  sim.set_notifier(spy);

  // Drive slots 0 … 30.
  for (unsigned s = 0; s <= 30; ++s) {
    advance(sim, s);
  }

  // Expect exactly 3 RACH indications — at slots 0, 10, 20.
  ASSERT_EQ(spy.rachs.size(), 3U);

  EXPECT_EQ(spy.rachs[0].slot.system_slot(), 0U);
  EXPECT_EQ(spy.rachs[1].slot.system_slot(), 10U);
  EXPECT_EQ(spy.rachs[2].slot.system_slot(), 20U);
}

TEST(fapi_dummy_rach_test, preamble_index_increments_per_ue)
{
  fapi_dummy_ue_config cfg;
  cfg.nof_ues                   = 3;
  cfg.ue_creation_stagger_slots = 10;

  fapi_dummy_ue_simulator sim(cfg);
  indications_spy         spy;
  sim.set_notifier(spy);

  for (unsigned s = 0; s <= 25; ++s) {
    advance(sim, s);
  }

  ASSERT_EQ(spy.rachs.size(), 3U);
  for (unsigned i = 0; i < 3; ++i) {
    ASSERT_EQ(spy.rachs[i].pdu.preambles.size(), 1U);
    EXPECT_EQ(spy.rachs[i].pdu.preambles[0].preamble_index, static_cast<uint8_t>(i));
  }
}

TEST(fapi_dummy_rach_test, no_rach_after_all_ues_created)
{
  fapi_dummy_ue_config cfg;
  cfg.nof_ues                   = 2;
  cfg.ue_creation_stagger_slots = 5;

  fapi_dummy_ue_simulator sim(cfg);
  indications_spy         spy;
  sim.set_notifier(spy);

  // Drive far past the last staggered slot.
  for (unsigned s = 0; s <= 50; ++s) {
    advance(sim, s);
  }

  EXPECT_EQ(spy.rachs.size(), 2U);
}

TEST(fapi_dummy_rach_test, zero_ues_produces_no_rach)
{
  fapi_dummy_ue_config cfg;
  cfg.nof_ues                   = 0;
  cfg.ue_creation_stagger_slots = 10;

  fapi_dummy_ue_simulator sim(cfg);
  indications_spy         spy;
  sim.set_notifier(spy);

  for (unsigned s = 0; s <= 20; ++s) {
    advance(sim, s);
  }

  EXPECT_TRUE(spy.rachs.empty());
}

TEST(fapi_dummy_rach_test, no_crash_without_notifier)
{
  fapi_dummy_ue_config cfg;
  cfg.nof_ues                   = 2;
  cfg.ue_creation_stagger_slots = 1;

  fapi_dummy_ue_simulator sim(cfg);
  // Notifier deliberately not set.

  EXPECT_NO_FATAL_FAILURE({
    for (unsigned s = 0; s <= 5; ++s) {
      advance(sim, s);
    }
  });
}
