#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_enterprise_ran OBJECT pusch/pusch_antenna_port_mapping.cpp)
enterprise_link_library(ocudu_ran ocudu_enterprise_ran)

add_to_exported_libs(ocudu_enterprise_ran)
