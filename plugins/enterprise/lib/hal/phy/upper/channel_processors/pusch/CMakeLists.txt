#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES 
  hw_accelerator_factories.cpp
  hw_accelerator_pusch_dec_impl.cpp
  hw_accelerator_pusch_dec_acc100_impl.cpp
)

add_library(ocudu_enterprise_hal_pusch OBJECT ${SOURCES})
enterprise_link_library(ocudu_hal_pusch  $<TARGET_OBJECTS:ocudu_enterprise_hal_pusch>)

target_include_directories(
    ocudu_enterprise_hal_pusch
    PRIVATE
    ${OCUDU_ENTERPRISE_SRC_DIR}/include
)
