#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_enterprise_mac OBJECT mac_sched/positioning_handler.cpp)

enterprise_link_library(ocudu_mac $<TARGET_OBJECTS:ocudu_enterprise_mac>)
target_include_directories(ocudu_enterprise_mac PUBLIC "${CMAKE_SOURCE_DIR}/lib/mac")
