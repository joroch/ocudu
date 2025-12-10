#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES 
  bbdev.cpp
  bbdev_acc.cpp
  bbdev_acc_factory.cpp
  bbdev_op_pool_factory.cpp
  ldpc/bbdev_ldpc_encoder.cpp
  ldpc/bbdev_ldpc_decoder.cpp
)

add_library(ocudu_enterprise_hal_bbdev OBJECT ${SOURCES})
enterprise_link_library(ocudu_hal_bbdev  $<TARGET_OBJECTS:ocudu_enterprise_hal_bbdev>)

target_include_directories(
    ocudu_enterprise_hal_bbdev
    PRIVATE
    ${OCUDU_ENTERPRISE_SRC_DIR}/include
)
