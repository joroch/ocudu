#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(ul_power_control_test pusch_power_control_test.cpp pucch_power_control_test.cpp)
target_link_libraries(ul_power_control_test
        ocudu_sched
        ocudu_support
        ocudulog
        scheduler_test_utils
        scheduler_test_suite
        fmt
        gtest
        gtest_main)
target_include_directories(ul_power_control_test
        PRIVATE
        ${CMAKE_SOURCE_DIR}
)
gtest_discover_tests(ul_power_control_test)