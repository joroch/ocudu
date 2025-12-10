#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(ocudu_enterprise_cu_cp_test
			   cu_cp_mocn_test.cpp
			   cu_cp_inter_cu_handover_test.cpp
			   cu_cp_nrppa_test.cpp
			   )
target_link_libraries(ocudu_enterprise_cu_cp_test
	ocudu_cu_cp
	cu_cp_test_helpers
	e1ap_test_helpers
	e1ap_test_doubles
	f1ap_test_helpers
	f1ap_test_doubles
	ngap_test_helpers
	ngap_test_doubles
	nrppa_test_doubles
	ocudu_support
	ocudulog
	fmt
	gtest
	gtest_main)
target_include_directories(ocudu_enterprise_cu_cp_test
	PRIVATE
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/lib/cu_cp
	${CMAKE_SOURCE_DIR}/lib/ngap
	${CMAKE_SOURCE_DIR}/lib/f1ap
	${CMAKE_SOURCE_DIR}/tests
	${OCUDU_ENTERPRISE_SRC_DIR}/include
	${OCUDU_ENTERPRISE_SRC_DIR}/lib/nrppa
	${OCUDU_ENTERPRISE_SRC_DIR}/tests
)
gtest_discover_tests(ocudu_enterprise_cu_cp_test)
