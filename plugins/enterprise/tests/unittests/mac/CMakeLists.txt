#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(positioning_handler_test positioning_handler_test.cpp)
target_link_libraries(positioning_handler_test ocudu_mac ocudu_support ocudulog gtest gtest_main)
target_include_directories(positioning_handler_test
        PRIVATE
        ${CMAKE_SOURCE_DIR}
)
add_test(positioning_handler_test positioning_handler_test)
