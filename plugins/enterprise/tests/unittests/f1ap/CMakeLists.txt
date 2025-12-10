#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "f1ap")

add_executable(ocudu_enterprise_f1ap_du_test
        f1ap_du_positioning_exchange_procedure_test.cpp
        f1ap_du_trp_information_exchange_procedure_test.cpp
        f1ap_du_positioning_measurement_procedure_test.cpp)

target_link_libraries(ocudu_enterprise_f1ap_du_test
        ocudu_f1ap_du
        f1ap_du_test_helpers
        f1ap_asn1
        ocudu_support
        ocudulog
        gtest
        gtest_main)
target_include_directories(ocudu_enterprise_f1ap_du_test
        PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/lib/f1ap
        ${CMAKE_SOURCE_DIR}/tests/unittests/f1ap/du
)
gtest_discover_tests(ocudu_enterprise_f1ap_du_test)
