#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        cu_cp_impl.cpp
        cell_meas_manager/cell_meas_manager_impl.cpp
        routines/amf_connections_setup_routine.cpp
        routines/amf_connections_removal_routine.cpp
        routines/mobility/inter_cu_handover_source_routine.cpp
        routines/mobility/inter_cu_handover_target_routine.cpp
        routines/mobility/inter_cu_handover_execution_target_routine.cpp
        routines/positioning/trp_information_exchange_routine.cpp)

add_library(ocudu_enterprise_cu_cp OBJECT ${SOURCES})

enterprise_link_library(ocudu_cu_cp $<TARGET_OBJECTS:ocudu_enterprise_cu_cp>)
enterprise_link_library(ocudu_cu_cp_cell_meas_manager  $<TARGET_OBJECTS:ocudu_enterprise_cu_cp>)
enterprise_link_library(ocudu_cu_cp_cell_meas_manager  $<TARGET_OBJECTS:nrppa_asn1>)

target_include_directories(
    ocudu_enterprise_cu_cp
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/cu_cp
    ${OCUDU_ENTERPRISE_SRC_DIR}/include
)
