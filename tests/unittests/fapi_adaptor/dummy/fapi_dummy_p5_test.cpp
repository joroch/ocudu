// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "lib/fapi_adaptor/dummy/fapi_dummy_sector.h"
#include "ocudu/fapi/common/error_code.h"
#include "ocudu/fapi/p5/p5_messages.h"
#include "ocudu/fapi/p5/p5_responses_notifier.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace ocudu::fapi_adaptor;

namespace {

/// Spy implementation of p5_responses_notifier that records every call.
class p5_notifier_spy : public fapi::p5_responses_notifier
{
public:
  void on_param_response(const fapi::param_response& msg) override
  {
    param_response_count++;
    last_param_error = msg.error_code;
  }

  void on_config_response(const fapi::config_response& msg) override
  {
    config_response_count++;
    last_config_error = msg.error_code;
  }

  void on_stop_indication(const fapi::stop_indication& msg) override { stop_indication_count++; }

  unsigned                        param_response_count  = 0;
  unsigned                        config_response_count = 0;
  unsigned                        stop_indication_count = 0;
  fapi::error_code_id             last_param_error{};
  fapi::error_code_id             last_config_error{};
};

} // namespace

class fapi_dummy_p5_test : public ::testing::Test
{
protected:
  fapi_dummy_p5_test() : sector(fapi_dummy_cell_config{})
  {
    sector.get_p5_sector_adaptor().set_p5_responses_notifier(spy);
  }

  p5_notifier_spy  spy;
  fapi_dummy_sector sector;
};

TEST_F(fapi_dummy_p5_test, param_request_returns_ok)
{
  sector.get_p5_sector_adaptor().get_p5_requests_gateway().send_param_request({});

  EXPECT_EQ(spy.param_response_count, 1U);
  EXPECT_EQ(spy.last_param_error, fapi::error_code_id::msg_ok);
}

TEST_F(fapi_dummy_p5_test, config_request_returns_ok)
{
  sector.get_p5_sector_adaptor().get_p5_requests_gateway().send_config_request({});

  EXPECT_EQ(spy.config_response_count, 1U);
  EXPECT_EQ(spy.last_config_error, fapi::error_code_id::msg_ok);
}

TEST_F(fapi_dummy_p5_test, start_request_fires_callback)
{
  bool start_called = false;
  // Access the p5 gateway directly through the sector to set the callback.
  // We reach the gateway via the P5 sector adaptor gateway reference and cast.
  auto& gw = static_cast<fapi_dummy_p5_gateway&>(
      sector.get_p5_sector_adaptor().get_p5_requests_gateway());
  gw.set_start_callback([&start_called]() { start_called = true; });

  sector.get_p5_sector_adaptor().get_p5_requests_gateway().send_start_request({});

  EXPECT_TRUE(start_called);
  // start_request has no P5 response — notifier should not be called.
  EXPECT_EQ(spy.param_response_count, 0U);
  EXPECT_EQ(spy.config_response_count, 0U);
  EXPECT_EQ(spy.stop_indication_count, 0U);
}

TEST_F(fapi_dummy_p5_test, stop_request_fires_callback_and_sends_stop_indication)
{
  bool stop_called = false;
  auto& gw = static_cast<fapi_dummy_p5_gateway&>(
      sector.get_p5_sector_adaptor().get_p5_requests_gateway());
  gw.set_stop_callback([&stop_called]() { stop_called = true; });

  sector.get_p5_sector_adaptor().get_p5_requests_gateway().send_stop_request({});

  EXPECT_TRUE(stop_called);
  EXPECT_EQ(spy.stop_indication_count, 1U);
}

TEST_F(fapi_dummy_p5_test, no_response_sent_without_notifier)
{
  // Create a sector without wiring a notifier — should not crash.
  fapi_dummy_sector bare_sector(fapi_dummy_cell_config{});
  EXPECT_NO_FATAL_FAILURE({
    bare_sector.get_p5_sector_adaptor().get_p5_requests_gateway().send_param_request({});
    bare_sector.get_p5_sector_adaptor().get_p5_requests_gateway().send_config_request({});
    bare_sector.get_p5_sector_adaptor().get_p5_requests_gateway().send_start_request({});
    bare_sector.get_p5_sector_adaptor().get_p5_requests_gateway().send_stop_request({});
  });
}
