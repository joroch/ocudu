#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(du_manager)

add_executable(ocudu_enterprise_du_high_test
			   du_high_positioning_test.cpp
			   )
target_link_libraries(ocudu_enterprise_du_high_test
	ocudu_du_high
    du_high_worker_manager
    ocudu_gateway
	f1ap_du_test_helpers
    gtest
    gtest_main)
target_include_directories(ocudu_enterprise_du_high_test
	PRIVATE
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/lib
	${CMAKE_SOURCE_DIR}/tests
	${OCUDU_ENTERPRISE_SRC_DIR}/include
	${OCUDU_ENTERPRISE_SRC_DIR}/lib
	${OCUDU_ENTERPRISE_SRC_DIR}/tests
)
gtest_discover_tests(ocudu_enterprise_du_high_test)
