#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(ran_resource_management)
add_subdirectory(procedures)

add_library(ocudu_enterprise_du_manager STATIC
    du_positioning_manager_impl.cpp
)
enterprise_link_library(ocudu_du_manager $<TARGET_OBJECTS:ocudu_enterprise_du_manager>)

target_include_directories(
        ocudu_enterprise_du_manager
        PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/du/du_high/du_manager/
)
