/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_sector_ofh_controller_impl.h"
#include "ocudu/support/ocudu_assert.h"

using namespace ocudu;

namespace {

class ofh_operation_controller_dummy : public ofh::operation_controller
{
public:
  void start() override { report_error("Dummy OFH sector operation controller cannot start the sector"); }
  void stop() override { report_error("Dummy OFH sector operation controller cannot stop the sector"); }
};

} // namespace

static ofh_operation_controller_dummy dummy_controller;

ru_sector_ofh_controller_impl::ru_sector_ofh_controller_impl(ocudulog::basic_logger& logger_) :
  logger(logger_), ofh_operation_controller(&dummy_controller)
{
}

void ru_sector_ofh_controller_impl::start()
{
  logger.info("Starting the operation of the Open Fronthaul interface");
  ofh_operation_controller->start();
  logger.info("Started the operation of the Open Fronthaul interface");
}

void ru_sector_ofh_controller_impl::stop()
{
  logger.info("Stopping the operation of the Open Fronthaul interface");
  ofh_operation_controller->stop();
  logger.info("Stopped the operation of the Open Fronthaul interface");
}
