// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

/// \file
/// \brief Integration test: FAPI dummy PHY driving DU-High until UEs attach.
///
/// Creates a fapi_dummy_sector (no real-time timing handler), wires it to an
/// o_du_high instance, then drives slots manually and asserts that the RACH
/// stagger produces Initial UL RRC Message Transfer messages on the F1 link.

#include "lib/fapi_adaptor/dummy/fapi_dummy_sector.h"
#include "test_utils/du_high_env_simulator.h"
#include "test_utils/du_high_sim_dependencies.h"
#include "tests/test_doubles/f1u/dummy_f1u_du_gateway.h"
#include "ocudu/asn1/f1ap/common.h"
#include "ocudu/asn1/f1ap/f1ap_pdu_contents.h"
#include "ocudu/du/du_high/du_high_clock_controller.h"
#include "ocudu/du/du_high/o_du_high_config.h"
#include "ocudu/du/du_high/o_du_high_factory.h"
#include "ocudu/du/du_operation_controller.h"
#include "ocudu/f1ap/f1ap_message.h"
#include "ocudu/fapi_adaptor/mac/mac_fapi_fastpath_adaptor.h"
#include "ocudu/fapi_adaptor/mac/mac_fapi_sector_fastpath_adaptor.h"
#include "ocudu/fapi_adaptor/mac/p7/mac_fapi_p7_sector_adaptor.h"
#include "ocudu/fapi_adaptor/mac/p7/mac_fapi_p7_sector_fastpath_adaptor.h"
#include "ocudu/ran/slot_point_extended.h"
#include "ocudu/support/io/io_broker_factory.h"
#include <gtest/gtest.h>
#include <thread>

using namespace ocudu;
using namespace odu;

namespace {

/// Test fixture: o_du_high driven by a fapi_dummy_sector.
class fapi_dummy_du_tester : public testing::Test
{
protected:
  static constexpr unsigned NOF_UES      = 2;
  static constexpr unsigned STAGGER_SLOTS = 5;

  fapi_dummy_du_tester() :
    cell_cfg([&] {
      fapi_adaptor::fapi_dummy_cell_config c;
      c.scs = subcarrier_spacing::kHz15;
      c.ue  = {NOF_UES, STAGGER_SLOTS};
      return c;
    }()),
    sector(cell_cfg),
    broker(create_io_broker(io_broker_type::epoll)),
    timer_ctrl(odu::create_du_high_clock_controller(timers, *broker, *workers.time_exec)),
    cu_notifier(workers.test_worker, /*cell_start_on_f1_setup=*/true),
    du_metrics(workers.test_worker),
    odu_hi(build_o_du_high()),
    next_slot(subcarrier_spacing::kHz15, 0U)
  {
    // Wire dummy sector to MAC FAPI adaptor.
    auto& mac_p7 = odu_hi->get_mac_fapi_fastpath_adaptor().get_sector_adaptor(0).get_p7_sector_adaptor();
    sector.set_p7_indications_notifier(mac_p7.get_p7_indications_notifier());
    sector.set_p7_slot_indication_notifier(mac_p7.get_p7_slot_indication_notifier());
  }

  ~fapi_dummy_du_tester() override
  {
    odu_hi->get_operation_controller().stop();
    timer_ctrl.reset();
    workers.stop();
  }

  /// Tick one slot: fire sector callback, wait for MAC to complete it, then flush the test thread.
  ///
  /// The spin-wait mirrors du_high_env_simulator::run_slot() — without it, slot indications
  /// are sent at CPU speed and overflow the MAC cell executor queue.
  void run_slot()
  {
    auto current_slot = next_slot.without_hyper_sfn();
    sector.on_new_slot(next_slot);
    ++next_slot;

    // Spin until mac_cell_processor signals completion for this slot via on_last_message().
    // The MAC calls on_last_message() even when the cell is inactive (returns immediately),
    // so this always terminates.
    static constexpr unsigned MAX_SPIN = 100000;
    for (unsigned i = 0; i < MAX_SPIN && !sector.is_slot_completed(current_slot); ++i) {
      workers.test_worker.run_pending_tasks();
      std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    // The MAC has finished processing current_slot. Flush any UL_TTI that was stored
    // during that slot's processing so that PUSCH/PUCCH ACKs use the correct slot.
    sector.flush_ul(current_slot);
    workers.test_worker.run_pending_tasks();
  }

  /// Advance slots until condition is met or max_slots reached.
  bool run_until(unique_function<bool()> cond, unsigned max_slots = 500)
  {
    for (unsigned i = 0; i < max_slots; ++i) {
      if (cond()) {
        return true;
      }
      run_slot();
    }
    return cond();
  }

  fapi_adaptor::fapi_dummy_cell_config cell_cfg;
  fapi_adaptor::fapi_dummy_sector      sector;

  du_high_worker_manager                workers;
  timer_manager                         timers;
  std::unique_ptr<io_broker>            broker;
  std::unique_ptr<mac_clock_controller> timer_ctrl;

  dummy_f1c_test_client     cu_notifier;
  cu_up_simulator           cu_up_sim;
  dummy_du_metrics_notifier du_metrics;
  null_mac_pcap             mac_pcap;
  null_rlc_pcap             rlc_pcap;

  std::unique_ptr<o_du_high> odu_hi;

  slot_point_extended next_slot;

private:
  std::unique_ptr<o_du_high> build_o_du_high()
  {
    odu::o_du_high_config cfg;
    cfg.du_hi = create_du_high_configuration();

    odu::o_du_high_dependencies deps;
    deps.du_hi.exec_mapper = workers.exec_mapper.get();
    deps.du_hi.f1c_client  = &cu_notifier;
    deps.du_hi.f1u_gw      = &cu_up_sim;
    deps.du_hi.du_notifier = &du_metrics;
    deps.du_hi.timer_ctrl  = timer_ctrl.get();
    deps.du_hi.mac_p       = &mac_pcap;
    deps.du_hi.rlc_p       = &rlc_pcap;

    odu::o_du_high_sector_dependencies sector_deps{
        .p5_gateway           = sector.get_p5_requests_gateway(),
        .p7_gateway           = sector.get_p7_requests_gateway(),
        .p7_last_req_notifier = sector.get_p7_last_request_notifier(),
        .timer_mng            = timers,
        .fapi_ctrl_executor   = workers.test_worker,
        .mac_ctrl_executor    = workers.exec_mapper->du_control_executor(),
        .fapi_logger          = ocudulog::fetch_basic_logger("FAPI"),
    };
    deps.sectors.push_back(sector_deps);

    auto odu = make_o_du_high(cfg, std::move(deps));
    odu->get_operation_controller().start();

    return odu;
  }
};

} // namespace

/// Drive 500 slots and verify that 2 UEs triggered F1 Initial UL RRC Message Transfer
/// (one at slot 0, one at slot 5 via RACH stagger).
TEST_F(fapi_dummy_du_tester, two_ues_trigger_initial_ul_rrc_via_rach_stagger)
{
  // Wait for F1 Setup complete (sent right after DU starts).
  bool f1_setup_done = run_until([this]() { return !cu_notifier.f1ap_ul_msgs.empty(); });
  ASSERT_TRUE(f1_setup_done) << "F1 Setup not seen within 500 slots";

  cu_notifier.f1ap_ul_msgs.clear();

  // Run until both UEs have sent Initial UL RRC Message Transfer (each RACH causes one).
  // UE 0 RACH at slot 0, UE 1 RACH at slot 5.  Allow up to 500 slots for the RA
  // procedure + RRC signalling to propagate to F1AP.
  auto count_init_ul_rrc = [this]() {
    unsigned count = 0;
    for (const auto& [_, msg] : cu_notifier.f1ap_ul_msgs) {
      if (msg.pdu.type() == asn1::f1ap::f1ap_pdu_c::types_opts::init_msg &&
          msg.pdu.init_msg().proc_code == ASN1_F1AP_ID_INIT_UL_RRC_MSG_TRANSFER) {
        ++count;
      }
    }
    return count;
  };

  bool two_ues_attached = run_until([&]() { return count_init_ul_rrc() >= NOF_UES; }, 500);

  EXPECT_TRUE(two_ues_attached) << "Only " << count_init_ul_rrc() << " Initial UL RRC transfers seen after 500 slots"
                                << " (expected " << NOF_UES << ")";
}
