# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI

add_library(ocudu_fapi_adaptor_dummy STATIC
    fapi_dummy_factory.cpp
    fapi_dummy_p5_gateway.cpp
    fapi_dummy_p7_gateway.cpp
    fapi_dummy_phy_adaptor.cpp
    fapi_dummy_sector.cpp
    fapi_dummy_timing_handler.cpp
    fapi_dummy_ue_simulator.cpp)

target_link_libraries(ocudu_fapi_adaptor_dummy PRIVATE ocudu_support)
