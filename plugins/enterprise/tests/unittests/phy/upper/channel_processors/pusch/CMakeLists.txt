#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(pusch_processor_phy_capabilities_test pusch_processor_phy_capabilities_test.cpp)
target_link_libraries(pusch_processor_phy_capabilities_test ocudu_channel_processors gtest gtest_main)
add_test(pusch_processor_phy_capabilities_test pusch_processor_phy_capabilities_test)
