#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_enterprise_channel_equalizer OBJECT channel_equalizer_generic_impl.cpp)
enterprise_link_library(ocudu_channel_equalizer ocudu_enterprise_channel_equalizer)
target_include_directories(ocudu_enterprise_channel_equalizer PRIVATE ${CMAKE_SOURCE_DIR}/lib/phy/upper/equalization)

add_to_exported_libs(ocudu_enterprise_channel_equalizer)

