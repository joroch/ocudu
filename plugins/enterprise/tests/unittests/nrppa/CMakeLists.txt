#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#


set(SOURCES
	nrppa_ue_context_test.cpp
	)

add_executable(ocudu_enterprise_nrppa_test ${SOURCES})
target_include_directories(ocudu_enterprise_nrppa_test
	PRIVATE
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/lib/cu_cp
	${OCUDU_ENTERPRISE_SRC_DIR}/include
	${OCUDU_ENTERPRISE_SRC_DIR}/lib/nrppa
	${OCUDU_ENTERPRISE_SRC_DIR}/tests
)
set_target_properties(ocudu_enterprise_nrppa_test PROPERTIES UNITY_BUILD ON)
target_link_libraries(ocudu_enterprise_nrppa_test nrppa_test_doubles ocudu_enterprise_nrppa ocudu_support nrppa_asn1 ocudu_ran ocudulog gtest gtest_main)
gtest_discover_tests(ocudu_enterprise_nrppa_test PROPERTIES "LABELS;nrppa")
