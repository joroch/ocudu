#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        procedures/positioning_information_exchange_procedure.cpp
        procedures/trp_information_exchange_procedure.cpp
        procedures/positioning_measurement_procedure.cpp
)

add_library(ocudu_enterprise_f1ap_du OBJECT ${SOURCES})
enterprise_link_library(ocudu_f1ap_du $<TARGET_OBJECTS:ocudu_enterprise_f1ap_du>)

target_include_directories(
        ocudu_enterprise_f1ap_du
        PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/f1ap
)
