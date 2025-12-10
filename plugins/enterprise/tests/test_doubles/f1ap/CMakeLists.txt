#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
    f1ap_test_message_validators.cpp
    f1ap_test_messages.cpp
)

add_library(ocudu_enterprise_f1ap_test_doubles OBJECT ${SOURCES})
enterprise_link_library(f1ap_test_doubles $<TARGET_OBJECTS:ocudu_enterprise_f1ap_test_doubles>)

target_include_directories(
    ocudu_enterprise_f1ap_test_doubles
    PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/test_doubles/f1ap
    ${OCUDU_ENTERPRISE_SRC_DIR}/lib
    ${OCUDU_ENTERPRISE_SRC_DIR}/include
)