/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/phy/upper/channel_processors/pusch/pusch_processor_phy_capabilities.h"
#include <gtest/gtest.h>

using namespace ocudu;

TEST(PuschProcessorPhyCapabilities, EnterpriseCapabilities)
{
  auto capabilities = get_pusch_processor_phy_capabilities();
  ASSERT_EQ(capabilities.max_nof_layers, 4);
}
