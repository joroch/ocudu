// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "lib/fapi_adaptor/dummy/fapi_dummy_sector.h"
#include "lib/fapi_adaptor/dummy/fapi_dummy_timing_handler.h"
#include "ocudu/fapi/p7/messages/slot_indication.h"
#include "ocudu/fapi/p7/p7_slot_indication_notifier.h"
#include "tests/unittests/support/task_executor_test_doubles.h"
#include <gtest/gtest.h>
#include <vector>

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

namespace {

/// Spy notifier that records every slot_indication received.
class slot_indication_spy : public fapi::p7_slot_indication_notifier
{
public:
  void on_slot_indication(const fapi::slot_indication& msg) override { indications.push_back(msg.slot); }

  std::vector<slot_point_extended> indications;
};

} // namespace

/// Helper: stop the handler and drain the executor so the stop-flag is observed by the loop task.
static void stop_and_drain(fapi_dummy_timing_handler& handler, manual_task_worker_always_enqueue_tasks& exec)
{
  handler.stop();
  exec.run_pending_tasks();
}

class fapi_dummy_timing_test : public ::testing::Test
{
protected:
  fapi_dummy_timing_test() : sector(fapi_dummy_cell_config{}), executor(128)
  {
    sector.get_p7_sector_adaptor().set_p7_slot_indication_notifier(spy);
  }

  slot_indication_spy                      spy;
  fapi_dummy_sector                        sector;
  manual_task_worker_always_enqueue_tasks  executor;
};

TEST_F(fapi_dummy_timing_test, no_advancement_when_clock_is_stationary)
{
  uint64_t simulated_slot = 0;
  auto     clock_fn       = [&simulated_slot]() -> uint64_t { return simulated_slot; };

  fapi_dummy_timing_handler handler(subcarrier_spacing::kHz15, executor, {&sector}, clock_fn);
  handler.start();

  // Run the loop once — slot has not advanced so no indication should fire.
  ASSERT_TRUE(executor.try_run_next());
  EXPECT_TRUE(spy.indications.empty());

  stop_and_drain(handler, executor);
}

TEST_F(fapi_dummy_timing_test, fires_one_indication_per_slot_advanced)
{
  uint64_t simulated_slot = 0;
  auto     clock_fn       = [&simulated_slot]() -> uint64_t { return simulated_slot; };

  fapi_dummy_timing_handler handler(subcarrier_spacing::kHz15, executor, {&sector}, clock_fn);
  handler.start();

  // Advance clock by 5 slots, then run the loop.
  simulated_slot = 5;
  ASSERT_TRUE(executor.try_run_next());

  ASSERT_EQ(spy.indications.size(), 5U);

  // Verify sequential slot numbers 1 … 5 (initial slot was 0).
  for (unsigned i = 0; i < 5; ++i) {
    EXPECT_EQ(spy.indications[i].system_slot(), i + 1U)
        << "Expected slot " << (i + 1) << " but got " << spy.indications[i].system_slot();
  }

  stop_and_drain(handler, executor);
}

TEST_F(fapi_dummy_timing_test, accumulates_indications_across_multiple_loop_iterations)
{
  uint64_t simulated_slot = 0;
  auto     clock_fn       = [&simulated_slot]() -> uint64_t { return simulated_slot; };

  fapi_dummy_timing_handler handler(subcarrier_spacing::kHz15, executor, {&sector}, clock_fn);
  handler.start();

  // First iteration: advance 3 slots.
  simulated_slot = 3;
  ASSERT_TRUE(executor.try_run_next());
  EXPECT_EQ(spy.indications.size(), 3U);

  // Second iteration: advance 2 more slots.
  simulated_slot = 5;
  ASSERT_TRUE(executor.try_run_next());
  EXPECT_EQ(spy.indications.size(), 5U);

  // Slot numbers should be 1, 2, 3, 4, 5.
  for (unsigned i = 0; i < 5; ++i) {
    EXPECT_EQ(spy.indications[i].system_slot(), i + 1U);
  }

  stop_and_drain(handler, executor);
}

TEST_F(fapi_dummy_timing_test, no_indications_fired_without_notifier)
{
  // Sector without a wired notifier — timing handler should not crash.
  fapi_dummy_sector bare_sector(fapi_dummy_cell_config{});

  uint64_t simulated_slot = 0;
  auto     clock_fn       = [&simulated_slot]() -> uint64_t { return simulated_slot; };

  fapi_dummy_timing_handler handler(subcarrier_spacing::kHz15, executor, {&bare_sector}, clock_fn);
  handler.start();

  simulated_slot = 3;
  EXPECT_NO_FATAL_FAILURE({ executor.try_run_next(); });

  stop_and_drain(handler, executor);
}
