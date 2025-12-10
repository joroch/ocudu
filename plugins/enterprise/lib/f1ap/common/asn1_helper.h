/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/asn1/f1ap/f1ap_ies.h"
#include "ocudu/f1ap/du/f1ap_du_positioning_handler.h"

namespace ocudu {

namespace asn1_helper {

asn1::f1ap::trp_info_s trp_info_to_asn1(const odu::du_trp_info& trp);

std::vector<odu::srs_carrier> from_asn1(const asn1::f1ap::srs_configuration_s& asn1cfg);

} // namespace asn1_helper
} // namespace ocudu
