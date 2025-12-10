#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

# NRPPa ASN1
add_library(nrppa_asn1 OBJECT nrppa/nrppa.cpp)
target_compile_options(nrppa_asn1 PRIVATE -Os -fno-exceptions)
target_link_libraries(nrppa_asn1 asn1_utils)

target_include_directories(
    nrppa_asn1
    PRIVATE
    ${OCUDU_ENTERPRISE_SRC_DIR}/include
)
