#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        nrppa_factory.cpp
        nrppa_helper.cpp
        nrppa_impl.cpp
        procedures/e_cid_measurement_initiation_procedure.cpp
        procedures/e_cid_measurement_termination_procedure.cpp
        procedures/measurement_procedure.cpp
        procedures/trp_information_exchange_procedure.cpp
        procedures/positioning_information_exchange_procedure.cpp
        procedures/positioning_activation_procedure.cpp
        ue_context/nrppa_ue_context.cpp
        du_context/nrppa_du_context.cpp
        meas_context/nrppa_meas_context.cpp
        )

add_library(ocudu_enterprise_nrppa OBJECT ${SOURCES})
enterprise_link_library(ocudu_nrppa  $<TARGET_OBJECTS:ocudu_enterprise_nrppa>)

target_include_directories(
    ocudu_enterprise_nrppa
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/nrppa
    ${OCUDU_ENTERPRISE_SRC_DIR}/include
)
