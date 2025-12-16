#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        gateways/xnc_network_gateway_factory.cpp
    )


add_library(ocudu_xnap STATIC ${SOURCES})

target_link_libraries(ocudu_xnap
	ocudu_cu_cp
	xnap_asn1
	ocudu_support)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
