# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI

set_directory_properties(PROPERTIES LABELS "fapi_adaptor")

add_executable(fapi_dummy_p5_test fapi_dummy_p5_test.cpp)
target_include_directories(fapi_dummy_p5_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(fapi_dummy_p5_test PRIVATE
    ocudu_fapi_adaptor_dummy
    ocudu_support
    gtest
    gtest_main)
gtest_discover_tests(fapi_dummy_p5_test)

add_executable(fapi_dummy_timing_test fapi_dummy_timing_test.cpp)
target_include_directories(fapi_dummy_timing_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(fapi_dummy_timing_test PRIVATE
    ocudu_fapi_adaptor_dummy
    ocudu_support
    gtest
    gtest_main)
gtest_discover_tests(fapi_dummy_timing_test)

add_executable(fapi_dummy_ul_ack_test fapi_dummy_ul_ack_test.cpp)
target_include_directories(fapi_dummy_ul_ack_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(fapi_dummy_ul_ack_test PRIVATE
    ocudu_fapi_adaptor_dummy
    ocudu_support
    gtest
    gtest_main)
gtest_discover_tests(fapi_dummy_ul_ack_test)

add_executable(fapi_dummy_rach_test fapi_dummy_rach_test.cpp)
target_include_directories(fapi_dummy_rach_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(fapi_dummy_rach_test PRIVATE
    ocudu_fapi_adaptor_dummy
    ocudu_support
    gtest
    gtest_main)
gtest_discover_tests(fapi_dummy_rach_test)
