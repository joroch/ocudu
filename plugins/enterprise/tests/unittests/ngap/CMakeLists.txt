#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(ocudu_enterprise_ngap_test
			   ngap_handover_preparation_procedure_test.cpp)
target_link_libraries(ocudu_enterprise_ngap_test
	ocudu_ngap
	ngap_test_helpers
	ocudu_cu_cp
	ngap_asn1
	ocudu_support
	ocudulog
	fmt
	gtest
	gtest_main)
target_include_directories(ocudu_enterprise_ngap_test
	PRIVATE
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/lib/ngap
)
gtest_discover_tests(ocudu_enterprise_ngap_test)
