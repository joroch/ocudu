#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_enterprise_sched OBJECT
        ue_context/ue_channel_state_manager.cpp
        support/pusch_power_controller.cpp
        support/pucch_power_controller.cpp
)
target_include_directories(ocudu_enterprise_sched PUBLIC "${CMAKE_SOURCE_DIR}/lib/scheduler")

enterprise_link_library(sched_support $<TARGET_OBJECTS:ocudu_enterprise_sched>)
enterprise_link_library(sched_ue_context $<TARGET_OBJECTS:ocudu_enterprise_sched>)
