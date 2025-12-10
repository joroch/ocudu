#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
    f1ap_cu_impl.cpp
    procedures/f1ap_trp_information_exchange_procedure.cpp
    procedures/f1ap_positioning_information_exchange_procedure.cpp
    procedures/f1ap_positioning_activation_procedure.cpp
    procedures/f1ap_positioning_measurement_procedure.cpp
    )

add_library(ocudu_enterprise_f1ap_cu OBJECT ${SOURCES})
enterprise_link_library(ocudu_f1ap_cu $<TARGET_OBJECTS:ocudu_enterprise_f1ap_cu>)

target_include_directories(
    ocudu_enterprise_f1ap_cu
    PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
)
