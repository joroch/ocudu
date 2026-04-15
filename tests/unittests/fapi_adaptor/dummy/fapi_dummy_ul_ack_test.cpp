// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

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

/// Spy notifier that records every indication received.
class indications_spy : public fapi::p7_indications_notifier
{
public:
  void on_rx_data_indication(const fapi::rx_data_indication& msg) override { rx_data.push_back(msg); }
  void on_crc_indication(const fapi::crc_indication& msg) override { crcs.push_back(msg); }
  void on_uci_indication(const fapi::uci_indication& msg) override { ucis.push_back(msg); }
  void on_srs_indication(const fapi::srs_indication&) override {}
  void on_rach_indication(const fapi::rach_indication&) override {}

  // Convenience: copy to own storage so spans remain valid after test manipulations.
  std::vector<fapi::crc_indication>       crcs;
  std::vector<fapi::rx_data_indication>   rx_data;
  std::vector<fapi::uci_indication>       ucis;
};

/// Build a UL_TTI.request containing \p n_pusch PUSCH PDUs with data TBs.
fapi::ul_tti_request make_pusch_request(slot_point slot, unsigned n_pusch, units::bytes tbs)
{
  fapi::ul_tti_request req{};
  req.slot = slot;
  for (unsigned i = 0; i < n_pusch; ++i) {
    fapi::ul_pusch_pdu pdu{};
    pdu.rnti   = to_rnti(0xC001 + i);
    pdu.handle = 100U + i;

    fapi::ul_pusch_data data{};
    data.harq_process_id = to_harq_id(i);
    data.tb_size         = tbs;
    data.rv_index        = 0;
    data.new_data        = true;
    pdu.pusch_data       = data;

    fapi::ul_tti_request_pdu wrapper{};
    wrapper.pdu = pdu;
    req.pdus.push_back(wrapper);
  }
  return req;
}

/// Build a UL_TTI.request containing one PUCCH format-0 PDU with \p n_harq HARQ bits.
fapi::ul_tti_request make_pucch_f0_request(slot_point slot, unsigned n_harq)
{
  fapi::ul_pucch_pdu pdu{};
  pdu.rnti   = to_rnti(0xAAAA);
  pdu.handle = 200U;

  fapi::ul_pucch_pdu_format_0 fmt0{};
  fmt0.bit_len_harq = units::bits(n_harq);
  pdu.format        = fmt0;

  fapi::ul_tti_request_pdu wrapper{};
  wrapper.pdu = pdu;

  fapi::ul_tti_request req{};
  req.slot = slot;
  req.pdus.push_back(wrapper);
  return req;
}

} // namespace

class fapi_dummy_ul_ack_test : public ::testing::Test
{
protected:
  fapi_dummy_ul_ack_test()
  {
    sim.set_notifier(spy);
  }

  indications_spy         spy;
  fapi_dummy_ue_simulator sim;
};

// ---------------------------------------------------------------------------
// PUSCH tests
// ---------------------------------------------------------------------------

TEST_F(fapi_dummy_ul_ack_test, two_pusch_pdus_generate_crc_and_rx_data)
{
  const slot_point slot{subcarrier_spacing::kHz15, 0, 5};
  const auto       req = make_pusch_request(slot, 2, units::bytes(100));

  sim.store_ul_tti(req);
  sim.on_new_slot(slot);

  ASSERT_EQ(spy.crcs.size(), 2U);
  ASSERT_EQ(spy.rx_data.size(), 2U);

  for (unsigned i = 0; i < 2; ++i) {
    EXPECT_TRUE(spy.crcs[i].pdu.tb_crc_status_ok);
    EXPECT_EQ(spy.crcs[i].pdu.handle, 100U + i);
    EXPECT_EQ(spy.crcs[i].pdu.rnti, to_rnti(0xC001 + i));
    EXPECT_EQ(spy.crcs[i].pdu.harq_id, to_harq_id(i));

    EXPECT_EQ(spy.rx_data[i].pdu.handle, 100U + i);
    EXPECT_EQ(spy.rx_data[i].pdu.rnti, to_rnti(0xC001 + i));
    EXPECT_EQ(spy.rx_data[i].pdu.transport_block.size(), 100U);
  }
}

TEST_F(fapi_dummy_ul_ack_test, no_indications_for_different_slot)
{
  const slot_point slot_n{subcarrier_spacing::kHz15, 0, 5};
  const slot_point slot_m{subcarrier_spacing::kHz15, 0, 6};

  sim.store_ul_tti(make_pusch_request(slot_n, 1, units::bytes(50)));
  sim.on_new_slot(slot_m); // different slot — nothing should fire

  EXPECT_TRUE(spy.crcs.empty());
  EXPECT_TRUE(spy.rx_data.empty());
}

TEST_F(fapi_dummy_ul_ack_test, no_indications_without_notifier)
{
  fapi_dummy_ue_simulator bare_sim;
  const slot_point        slot{subcarrier_spacing::kHz15, 0, 0};

  bare_sim.store_ul_tti(make_pusch_request(slot, 2, units::bytes(50)));
  EXPECT_NO_FATAL_FAILURE({ bare_sim.on_new_slot(slot); });
}

TEST_F(fapi_dummy_ul_ack_test, no_double_fire_after_pop)
{
  const slot_point slot{subcarrier_spacing::kHz15, 0, 3};
  sim.store_ul_tti(make_pusch_request(slot, 1, units::bytes(50)));

  sim.on_new_slot(slot); // first call — should fire
  EXPECT_EQ(spy.crcs.size(), 1U);

  sim.on_new_slot(slot); // second call for same slot — buffer already cleared
  EXPECT_EQ(spy.crcs.size(), 1U);
}

// ---------------------------------------------------------------------------
// PUCCH tests
// ---------------------------------------------------------------------------

TEST_F(fapi_dummy_ul_ack_test, pucch_format0_generates_uci_with_ack)
{
  const slot_point slot{subcarrier_spacing::kHz15, 0, 7};

  sim.store_ul_tti(make_pucch_f0_request(slot, 1));
  sim.on_new_slot(slot);

  ASSERT_EQ(spy.ucis.size(), 1U);
  const auto* f01 = std::get_if<fapi::uci_pucch_pdu_format_0_1>(&spy.ucis[0].pdu);
  ASSERT_NE(f01, nullptr);
  EXPECT_EQ(f01->rnti, to_rnti(0xAAAA));
  EXPECT_EQ(f01->pucch_format, fapi::uci_pucch_pdu_format_0_1::format_type::format_0);
  ASSERT_TRUE(f01->harq.has_value());
  ASSERT_EQ(f01->harq->harq_values.size(), 1U);
  EXPECT_EQ(f01->harq->harq_values[0], uci_pucch_f0_or_f1_harq_values::ack);
}
