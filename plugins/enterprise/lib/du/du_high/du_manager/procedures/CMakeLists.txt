#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        du_ue_positioning_info_procedure.cpp
        du_positioning_measurement_procedure.cpp
)

add_library(ocudu_enterprise_du_manager_procedures OBJECT ${SOURCES})
enterprise_link_library(ocudu_du_manager_procedures $<TARGET_OBJECTS:ocudu_enterprise_du_manager_procedures>)

target_include_directories(
        ocudu_enterprise_du_manager_procedures
        PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/lib/du/du_high/du_manager/
)
