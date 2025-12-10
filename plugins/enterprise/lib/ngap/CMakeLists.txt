#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
    ngap_impl.cpp
    procedures/ngap_handover_preparation_procedure.cpp
    procedures/ngap_handover_resource_allocation_procedure.cpp
    procedures/ngap_dl_ran_status_transfer_procedure.cpp)

add_library(ocudu_enterprise_ngap OBJECT ${SOURCES})
enterprise_link_library(ocudu_ngap  $<TARGET_OBJECTS:ocudu_enterprise_ngap>)

target_include_directories(
    ocudu_enterprise_ngap
    PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/ngap
)
