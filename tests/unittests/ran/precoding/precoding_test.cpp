// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "ocudu/ran/precoding/precoding_codebooks.h"
#include <gtest/gtest.h>

using namespace ocudu;

/// Tolerance for floating-point arithmetics equality comparisons.
static constexpr float TOLERANCE_FLOATING_POINT = 1e-5;

/// Assert that two float-based complex values are equal.
#define ASSERT_CF_EQ(val1, val2)                                                                                       \
  ASSERT_NEAR((val1).real(), (val2).real(), TOLERANCE_FLOATING_POINT);                                                 \
  ASSERT_NEAR((val1).imag(), (val2).imag(), TOLERANCE_FLOATING_POINT);

// Test the precoding matrix generated for a one layer transmission using one antenna port.
TEST(precoding_test, OneLayerOnePort)
{
  static constexpr unsigned max_nof_ports = precoding_constants::MAX_NOF_PORTS;
  for (unsigned nof_ports = 1; nof_ports != max_nof_ports; ++nof_ports) {
    for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
      precoding_weight_matrix precoding = make_one_layer_one_port(nof_ports, i_port);

      // Assert precoding matrix dimensions.
      ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
      ASSERT_EQ(precoding.get_nof_layers(), 1);

      // Assert precoding matrix coefficients.
      for (unsigned i_port_2 = 0; i_port_2 != nof_ports; ++i_port_2) {
        cf_t expected = (i_port_2 == i_port) ? cf_t(1.0f, 0.0f) : cf_t(0.0f, 0.0f);
        ASSERT_CF_EQ(expected, precoding.get_coefficient(0, i_port_2));
      }
    }
  }
}

// Test the precoding matrix generated for a one layer transmission using the maximum number of ports.
TEST(precoding_test, OneLayerAllPorts)
{
  static constexpr unsigned max_nof_ports = precoding_constants::MAX_NOF_PORTS;
  for (unsigned nof_ports = 1; nof_ports != max_nof_ports; ++nof_ports) {
    precoding_weight_matrix precoding = make_one_layer_all_ports(nof_ports);

    // Assert precoding matrix dimensions.
    ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
    ASSERT_EQ(precoding.get_nof_layers(), 1);

    // Assert precoding matrix coefficients.
    for (unsigned i_port_2 = 0; i_port_2 != nof_ports; ++i_port_2) {
      cf_t expected = cf_t(1.0F / std::sqrt(static_cast<float>(nof_ports)), 0.0f);
      ASSERT_CF_EQ(expected, precoding.get_coefficient(0, i_port_2));
    }
  }
}

// Test the precoding matrix generated for the maximum number of transmission layers using the maximum number of ports,
// with an identity precoding matrix.
TEST(precoding_test, Identity)
{
  static constexpr unsigned max_nof_layers = precoding_constants::MAX_NOF_LAYERS;
  for (unsigned nof_layers = 1; nof_layers != max_nof_layers; ++nof_layers) {
    precoding_weight_matrix precoding = make_identity(nof_layers);

    // Assert precoding matrix dimensions.
    ASSERT_EQ(precoding.get_nof_ports(), nof_layers);
    ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

    // Assert precoding matrix coefficients.
    for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
      for (unsigned i_port = 0; i_port != nof_layers; ++i_port) {
        cf_t expected =
            (i_layer == i_port) ? cf_t(1.0f / std::sqrt(static_cast<float>(nof_layers)), 0.0f) : cf_t(0.0f, 0.0f);
        ASSERT_CF_EQ(expected, precoding.get_coefficient(i_layer, i_port));
      }
    }
  }
}

// Test the precoding matrix generated for a transmission using one layer and two antenna ports.
TEST(precoding_test, OneLayerTwoPorts)
{
  // List of precoding matrix coefficients for one layer and two antenna ports indexed by the codebook index, as per
  // TS 38.214 Table 5.2.2.2.1-1.
  static constexpr cf_t coefficients[4][2] = {{cf_t(1.0f, 0.0f), cf_t(1.0f, 0.0f)},
                                              {cf_t(1.0f, 0.0f), cf_t(0.0f, 1.0f)},
                                              {cf_t(1.0f, 0.0f), cf_t(-1.0f, 0.0f)},
                                              {cf_t(1.0f, 0.0f), cf_t(0.0f, -1.0f)}};

  static constexpr float norm_factor = M_SQRT1_2f;

  for (unsigned i_codebook = 0; i_codebook != 4; ++i_codebook) {
    precoding_weight_matrix precoding = make_one_layer_two_ports(i_codebook);

    // Assert precoding matrix dimensions.
    ASSERT_EQ(precoding.get_nof_ports(), 2);
    ASSERT_EQ(precoding.get_nof_layers(), 1);

    // Assert first port coefficient.
    ASSERT_CF_EQ(precoding.get_coefficient(0, 0), coefficients[i_codebook][0] * norm_factor);
    // Assert second port coefficient.
    ASSERT_CF_EQ(precoding.get_coefficient(0, 1), coefficients[i_codebook][1] * norm_factor);
  }
}

// Test the precoding matrix generated for a transmission using two layers and two antenna ports.
TEST(precoding_test, TwoLayerTwoPorts)
{
  // List of precoding matrix coefficients for two layer and two antenna ports indexed by the codebook index and layer,
  // as per TS 38.214 Table 5.2.2.2.1-1.
  static constexpr cf_t coefficients[2][2][2] = {
      {{cf_t(1.0f, 0.0f), cf_t(1.0f, 0.0f)}, {cf_t(1.0f, 0.0f), cf_t(-1.0f, 0.0f)}},
      {{cf_t(1.0f, 0.0f), cf_t(0.0f, 1.0f)}, {cf_t(1.0f, 0.0f), cf_t(0.0f, -1.0f)}},
  };

  static constexpr float norm_factor = 0.5f;

  for (unsigned i_codebook = 0; i_codebook != 2; ++i_codebook) {
    precoding_weight_matrix precoding = make_two_layer_two_ports(i_codebook);

    // Assert precoding matrix dimensions.
    ASSERT_EQ(precoding.get_nof_ports(), 2);
    ASSERT_EQ(precoding.get_nof_layers(), 2);

    // Assert first layer coefficients.
    ASSERT_CF_EQ(precoding.get_coefficient(0, 0), coefficients[i_codebook][0][0] * norm_factor);
    ASSERT_CF_EQ(precoding.get_coefficient(0, 1), coefficients[i_codebook][0][1] * norm_factor);

    // Assert second layer coefficients.
    ASSERT_CF_EQ(precoding.get_coefficient(1, 0), coefficients[i_codebook][1][0] * norm_factor);
    ASSERT_CF_EQ(precoding.get_coefficient(1, 1), coefficients[i_codebook][1][1] * norm_factor);
  }
}

// Test the precoding matrix generation for a transmission using one layer and four Type1 Single-Panel Mode1 antenna
// ports.
TEST(precoding_test, Type1SinglePanelMode1_OneLayerFourPorts)
{
  // Antenna configuration parameters, as per TS 38.214 Section 5.2.2.2.1.
  static constexpr unsigned N1             = 2;
  static constexpr unsigned O1             = 4;
  static constexpr unsigned nof_beams      = O1 * N1;
  static constexpr unsigned nof_pol_shifts = 4;
  static constexpr unsigned nof_ports      = 2 * N1;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
      precoding_weight_matrix precoding = make_one_layer_four_ports_type1_sp_mode1(i_1_1, i_2);

      // Assert precoding matrix dimensions.
      ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
      ASSERT_EQ(precoding.get_nof_layers(), 1);

      // Assert each coefficient independently from the spec formula. The precoding vector is
      // W = scaling * [v ; phi * v], where v[k] = exp(j * 2 * pi * i_1_1 * k / (O1 * N1)) is the beam steering
      // vector element and phi = exp(j * pi/2 * i_2) is the co-phasing factor between polarization groups.
      for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
        // Antenna element index within the polarization group.
        unsigned element       = i_port % N1;
        bool     is_second_pol = (i_port >= N1);

        // Beam steering phase for this element: 2 * pi * i_1_1 * element / (O1 * N1).
        float beam_phase =
            2.0F * static_cast<float>(M_PI) * static_cast<float>(i_1_1 * element) / static_cast<float>(nof_beams);

        // Co-phasing factor phase: pi/2 * i_2 for the second polarization group, zero otherwise.
        float pol_phase = is_second_pol ? (static_cast<float>(M_PI_2) * static_cast<float>(i_2)) : 0.0F;

        cf_t expected = scaling * std::polar(1.0F, beam_phase + pol_phase);
        ASSERT_CF_EQ(precoding.get_coefficient(0, i_port), expected);
      }
    }
  }
}

// Test the precoding matrix generation for a transmission using two layers and four Type1 Single-Panel Mode1 antenna
// ports.
TEST(precoding_test, Type1SinglePanelMode1_TwoLayerFourPorts)
{
  // Antenna configuration parameters, as per TS 38.214 Table 5.2.2.2.1-6.
  static constexpr unsigned N1             = 2;
  static constexpr unsigned O1             = 4;
  static constexpr unsigned nof_beams      = O1 * N1;
  static constexpr unsigned nof_layers     = 2;
  static constexpr unsigned nof_ports      = 2 * N1;
  static constexpr unsigned nof_pol_shifts = 2;
  static constexpr unsigned nof_offsets    = 2;
  // Beam offset between layers, as per TS 38.214 Table 5.2.2.2.1-4.
  static constexpr unsigned k1      = O1;
  static const float        scaling = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_1_3 = 0; i_1_3 != nof_offsets; ++i_1_3) {
      for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
        precoding_weight_matrix precoding = make_two_layer_four_ports_type1_sp_mode1(i_1_1, i_1_3, i_2);

        // Assert precoding matrix dimensions.
        ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
        ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

        // Per-layer beam indices and polarization phases, as per TS 38.214 Table 5.2.2.2.1-6.
        // Layer 0: beam l = i_1_1,      phi = pi/2 * i_2.
        // Layer 1: beam l = i_1_1 + k1, phi = pi/2 * i_2 + pi.
        float    phi           = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
        unsigned layer_beam[2] = {i_1_1, i_1_1 + k1};
        float    layer_pol[2]  = {phi, phi + static_cast<float>(M_PI)};

        for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
          for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
            unsigned element       = i_port % N1;
            bool     is_second_pol = (i_port >= N1);

            float beam_phase = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam[i_layer] * element) /
                               static_cast<float>(nof_beams);
            float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

            cf_t expected = scaling * std::polar(1.0F, beam_phase + pol_phase);
            ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
          }
        }
      }
    }
  }
}

// Test the precoding matrix generation for a transmission using three layers and four Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_ThreeLayerFourPorts)
{
  // Antenna configuration parameters, as per TS 38.214 Table 5.2.2.2.1-7.
  static constexpr unsigned N1             = 2;
  static constexpr unsigned O1             = 4;
  static constexpr unsigned nof_beams      = O1 * N1;
  static constexpr unsigned nof_layers     = 3;
  static constexpr unsigned nof_ports      = 2 * N1;
  static constexpr unsigned nof_pol_shifts = 2;
  // Beam offset between layers, as per TS 38.214 Table 5.2.2.2.1-4.
  static constexpr unsigned k1      = O1;
  static const float        scaling = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
      precoding_weight_matrix precoding = make_three_layer_four_ports_type1_sp(i_1_1, i_2);

      // Assert precoding matrix dimensions.
      ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
      ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

      // Per-layer beam indices and polarization phases, as per TS 38.214 Table 5.2.2.2.1-7.
      // Layer 0: beam l = i_1_1,      phi = pi/2 * i_2.
      // Layer 1: beam l = i_1_1 + k1, phi = pi/2 * i_2.
      // Layer 2: beam l = i_1_1,      phi = pi/2 * i_2 + pi.
      float    phi           = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
      unsigned layer_beam[3] = {i_1_1, i_1_1 + k1, i_1_1};
      float    layer_pol[3]  = {phi, phi, phi + static_cast<float>(M_PI)};

      for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
        for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
          unsigned element       = i_port % N1;
          bool     is_second_pol = (i_port >= N1);

          float beam_phase = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam[i_layer] * element) /
                             static_cast<float>(nof_beams);
          float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

          cf_t expected = scaling * std::polar(1.0F, beam_phase + pol_phase);
          ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
        }
      }
    }
  }
}

// Test the precoding matrix generation for a transmission using four layers and four Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_FourLayerFourPorts)
{
  // Antenna configuration parameters, as per TS 38.214 Table 5.2.2.2.1-8.
  static constexpr unsigned N1             = 2;
  static constexpr unsigned O1             = 4;
  static constexpr unsigned nof_beams      = O1 * N1;
  static constexpr unsigned nof_layers     = 4;
  static constexpr unsigned nof_ports      = 2 * N1;
  static constexpr unsigned nof_pol_shifts = 2;
  // Beam offset between layers, as per TS 38.214 Table 5.2.2.2.1-4.
  static constexpr unsigned k1      = O1;
  static const float        scaling = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
      precoding_weight_matrix precoding = make_four_layer_four_ports_type1_sp(i_1_1, i_2);

      // Assert precoding matrix dimensions.
      ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
      ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

      // Per-layer beam indices and polarization phases, as per TS 38.214 Table 5.2.2.2.1-8.
      // Layer 0: beam l = i_1_1,      phi = pi/2 * i_2.
      // Layer 1: beam l = i_1_1 + k1, phi = pi/2 * i_2.
      // Layer 2: beam l = i_1_1,      phi = pi/2 * i_2 + pi.
      // Layer 3: beam l = i_1_1 + k1, phi = pi/2 * i_2 + pi.
      float    phi           = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
      unsigned layer_beam[4] = {i_1_1, i_1_1 + k1, i_1_1, i_1_1 + k1};
      float    layer_pol[4]  = {phi, phi, phi + static_cast<float>(M_PI), phi + static_cast<float>(M_PI)};

      for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
        for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
          unsigned element       = i_port % N1;
          bool     is_second_pol = (i_port >= N1);

          float beam_phase = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam[i_layer] * element) /
                             static_cast<float>(nof_beams);
          float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

          cf_t expected = scaling * std::polar(1.0F, beam_phase + pol_phase);
          ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
        }
      }
    }
  }
}

// Test the precoding matrix generation for one layer and eight 1D Type1 Single-Panel Mode1 antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_OneLayerEightPorts1D)
{
  // Antenna configuration: N1=4, N2=1, as per TS 38.214 Table 5.2.2.2.1-5.
  static constexpr unsigned N1             = 4;
  static constexpr unsigned O1             = 4;
  static constexpr unsigned nof_beams      = O1 * N1;
  static constexpr unsigned nof_ports      = 2 * N1;
  static constexpr unsigned nof_pol_shifts = 4;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
      precoding_weight_matrix precoding = make_one_layer_eight_ports_type1_sp_mode1(i_1_1, i_2);

      ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
      ASSERT_EQ(precoding.get_nof_layers(), 1);

      // Layer 0: beam l = i_1_1, phi = pi/2 * i_2.
      for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
        unsigned element       = i_port % N1;
        bool     is_second_pol = (i_port >= N1);

        float beam_phase =
            2.0F * static_cast<float>(M_PI) * static_cast<float>(i_1_1 * element) / static_cast<float>(nof_beams);
        float pol_phase = is_second_pol ? (static_cast<float>(M_PI_2) * static_cast<float>(i_2)) : 0.0F;

        cf_t expected = scaling * std::polar(1.0F, beam_phase + pol_phase);
        ASSERT_CF_EQ(precoding.get_coefficient(0, i_port), expected);
      }
    }
  }
}

// Test the precoding matrix generation for two layers and eight 1D Type1 Single-Panel Mode1 antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_TwoLayerEightPorts1D)
{
  // Antenna configuration: N1=4, N2=1, as per TS 38.214 Table 5.2.2.2.1-6.
  static constexpr unsigned N1             = 4;
  static constexpr unsigned O1             = 4;
  static constexpr unsigned nof_beams      = O1 * N1;
  static constexpr unsigned nof_layers     = 2;
  static constexpr unsigned nof_ports      = 2 * N1;
  static constexpr unsigned nof_pol_shifts = 2;
  static constexpr unsigned nof_offsets    = 4;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_1_3 = 0; i_1_3 != nof_offsets; ++i_1_3) {
      for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
        precoding_weight_matrix precoding = make_two_layer_eight_ports_type1_sp_mode1(i_1_1, i_1_3, i_2);

        ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
        ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

        // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-6.
        // Layer 0: beam l = i_1_1, phi = pi/2 * i_2.
        // Layer 1: beam l = i_1_1 + k1, phi = pi/2 * i_2 + pi.
        unsigned k1            = i_1_3 * O1;
        float    phi           = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
        unsigned layer_beam[2] = {i_1_1, i_1_1 + k1};
        float    layer_pol[2]  = {phi, phi + static_cast<float>(M_PI)};

        for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
          for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
            unsigned element       = i_port % N1;
            bool     is_second_pol = (i_port >= N1);

            float beam_phase = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam[i_layer] * element) /
                               static_cast<float>(nof_beams);
            float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

            cf_t expected = scaling * std::polar(1.0F, beam_phase + pol_phase);
            ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
          }
        }
      }
    }
  }
}

// Test the precoding matrix generation for three layers and eight 1D Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_ThreeLayerEightPorts1D)
{
  // Antenna configuration: N1=4, N2=1, as per TS 38.214 Table 5.2.2.2.1-7.
  static constexpr unsigned N1             = 4;
  static constexpr unsigned O1             = 4;
  static constexpr unsigned nof_beams      = O1 * N1;
  static constexpr unsigned nof_layers     = 3;
  static constexpr unsigned nof_ports      = 2 * N1;
  static constexpr unsigned nof_pol_shifts = 2;
  static constexpr unsigned nof_offsets    = 3;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_1_3 = 0; i_1_3 != nof_offsets; ++i_1_3) {
      for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
        precoding_weight_matrix precoding = make_three_layer_eight_ports_type1_sp(i_1_1, i_1_3, i_2);

        ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
        ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

        // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-7.
        // Layer 0: beam l = i_1_1, phi = pi/2 * i_2.
        // Layer 1: beam l = i_1_1 + k1, phi = pi/2 * i_2.
        // Layer 2: beam l = i_1_1, phi = pi/2 * i_2 + pi.
        unsigned k1            = (i_1_3 + 1) * O1;
        float    phi           = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
        unsigned layer_beam[3] = {i_1_1, i_1_1 + k1, i_1_1};
        float    layer_pol[3]  = {phi, phi, phi + static_cast<float>(M_PI)};

        for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
          for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
            unsigned element       = i_port % N1;
            bool     is_second_pol = (i_port >= N1);

            float beam_phase = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam[i_layer] * element) /
                               static_cast<float>(nof_beams);
            float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

            cf_t expected = scaling * std::polar(1.0F, beam_phase + pol_phase);
            ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
          }
        }
      }
    }
  }
}

// Test the precoding matrix generation for four layers and eight 1D Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_FourLayerEightPorts1D)
{
  // Antenna configuration: N1=4, N2=1, as per TS 38.214 Table 5.2.2.2.1-8.
  static constexpr unsigned N1             = 4;
  static constexpr unsigned O1             = 4;
  static constexpr unsigned nof_beams      = O1 * N1;
  static constexpr unsigned nof_layers     = 4;
  static constexpr unsigned nof_ports      = 2 * N1;
  static constexpr unsigned nof_pol_shifts = 2;
  static constexpr unsigned nof_offsets    = 3;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_1_3 = 0; i_1_3 != nof_offsets; ++i_1_3) {
      for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
        precoding_weight_matrix precoding = make_four_layer_eight_ports_type1_sp(i_1_1, i_1_3, i_2);

        ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
        ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

        // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-8.
        // Layer 0: beam l = i_1_1, phi = pi/2 * i_2.
        // Layer 1: beam l = i_1_1 + k1, phi = pi/2 * i_2.
        // Layer 2: beam l = i_1_1, phi = pi/2 * i_2 + pi.
        // Layer 3: beam l = i_1_1 + k1, phi = pi/2 * i_2 + pi.
        unsigned k1            = (i_1_3 + 1) * O1;
        float    phi           = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
        unsigned layer_beam[4] = {i_1_1, i_1_1 + k1, i_1_1, i_1_1 + k1};
        float    layer_pol[4]  = {phi, phi, phi + static_cast<float>(M_PI), phi + static_cast<float>(M_PI)};

        for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
          for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
            unsigned element       = i_port % N1;
            bool     is_second_pol = (i_port >= N1);

            float beam_phase = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam[i_layer] * element) /
                               static_cast<float>(nof_beams);
            float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

            cf_t expected = scaling * std::polar(1.0F, beam_phase + pol_phase);
            ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
          }
        }
      }
    }
  }
}

// Test the precoding matrix generation for five layers and eight 1D Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_FiveLayerEightPorts1D)
{
  // Antenna configuration: N1=4, N2=1, as per TS 38.214 Table 5.2.2.2.1-9.
  static constexpr unsigned N1             = 4;
  static constexpr unsigned O1             = 4;
  static constexpr unsigned nof_beams      = O1 * N1;
  static constexpr unsigned nof_layers     = 5;
  static constexpr unsigned nof_ports      = 2 * N1;
  static constexpr unsigned nof_pol_shifts = 2;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
      precoding_weight_matrix precoding = make_five_layer_eight_ports_type1_sp(i_1_1, i_2);

      ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
      ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

      // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-9.
      // Layer 0: beam l = i_1_1, phi = pi/2 * i_2.
      // Layer 1: beam l = i_1_1, phi = pi/2 * i_2 + pi.
      // Layer 2: beam l = i_1_1 + O1, phi = 0.
      // Layer 3: beam l = i_1_1 + O1, phi = pi.
      // Layer 4: beam l = i_1_1 + 2 * O1, phi = 0.
      float    phi           = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
      unsigned layer_beam[5] = {i_1_1, i_1_1, i_1_1 + O1, i_1_1 + O1, i_1_1 + 2 * O1};
      float    layer_pol[5]  = {phi, phi + static_cast<float>(M_PI), 0.0F, static_cast<float>(M_PI), 0.0F};

      for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
        for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
          unsigned element       = i_port % N1;
          bool     is_second_pol = (i_port >= N1);

          float beam_phase = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam[i_layer] * element) /
                             static_cast<float>(nof_beams);
          float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

          cf_t expected = scaling * std::polar(1.0F, beam_phase + pol_phase);
          ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
        }
      }
    }
  }
}

// Test the precoding matrix generation for six layers and eight 1D Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_SixLayerEightPorts1D)
{
  // Antenna configuration: N1=4, N2=1, as per TS 38.214 Table 5.2.2.2.1-10.
  static constexpr unsigned N1             = 4;
  static constexpr unsigned O1             = 4;
  static constexpr unsigned nof_beams      = O1 * N1;
  static constexpr unsigned nof_layers     = 6;
  static constexpr unsigned nof_ports      = 2 * N1;
  static constexpr unsigned nof_pol_shifts = 2;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
      precoding_weight_matrix precoding = make_six_layer_eight_ports_type1_sp(i_1_1, i_2);

      ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
      ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

      // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-10.
      // Layer 0: beam l = i_1_1, phi = pi/2 * i_2.
      // Layer 1: beam l = i_1_1, phi = pi/2 * i_2 + pi.
      // Layer 2: beam l = i_1_1 + O1, phi = 0.
      // Layer 3: beam l = i_1_1 + O1, phi = pi.
      // Layer 4: beam l = i_1_1 + 2 * O1, phi = 0.
      // Layer 5: beam l = i_1_1 + 2 * O1, phi = pi.
      float    phi           = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
      unsigned layer_beam[6] = {i_1_1, i_1_1, i_1_1 + O1, i_1_1 + O1, i_1_1 + 2 * O1, i_1_1 + 2 * O1};
      float    layer_pol[6]  = {
          phi, phi + static_cast<float>(M_PI), 0.0F, static_cast<float>(M_PI), 0.0F, static_cast<float>(M_PI)};

      for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
        for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
          unsigned element       = i_port % N1;
          bool     is_second_pol = (i_port >= N1);

          float beam_phase = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam[i_layer] * element) /
                             static_cast<float>(nof_beams);
          float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

          cf_t expected = scaling * std::polar(1.0F, beam_phase + pol_phase);
          ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
        }
      }
    }
  }
}

// Test the precoding matrix generation for seven layers and eight 1D Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_SevenLayerEightPorts1D)
{
  // Antenna configuration: N1=4, N2=1, as per TS 38.214 Table 5.2.2.2.1-11.
  static constexpr unsigned N1             = 4;
  static constexpr unsigned O1             = 4;
  static constexpr unsigned nof_beams      = O1 * N1 / 2;
  static constexpr unsigned nof_layers     = 7;
  static constexpr unsigned nof_ports      = 2 * N1;
  static constexpr unsigned nof_pol_shifts = 2;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
      precoding_weight_matrix precoding = make_seven_layer_eight_ports_type1_sp(i_1_1, i_2);

      ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
      ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

      // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-11.
      // Layer 0: beam l = i_1_1, phi = pi/2 * i_2.
      // Layer 1: beam l = i_1_1, phi = pi/2 * i_2 + pi.
      // Layer 2: beam l = i_1_1 + O1, phi = pi/2 * i_2.
      // Layer 3: beam l = i_1_1 + 2 * O1, phi = 0.
      // Layer 4: beam l = i_1_1 + 2 * O1, phi = pi.
      // Layer 5: beam l = i_1_1 + 3 * O1, phi = 0.
      // Layer 6: beam l = i_1_1 + 3 * O1, phi = pi.
      float    phi           = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
      unsigned layer_beam[7] = {
          i_1_1, i_1_1, i_1_1 + O1, i_1_1 + 2 * O1, i_1_1 + 2 * O1, i_1_1 + 3 * O1, i_1_1 + 3 * O1};
      float layer_pol[7] = {
          phi, phi + static_cast<float>(M_PI), phi, 0.0F, static_cast<float>(M_PI), 0.0F, static_cast<float>(M_PI)};

      for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
        for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
          unsigned element       = i_port % N1;
          bool     is_second_pol = (i_port >= N1);

          float beam_phase = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam[i_layer] * element) /
                             static_cast<float>(nof_beams);
          float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

          cf_t expected = scaling * std::polar(1.0F, beam_phase + pol_phase);
          ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
        }
      }
    }
  }
}

// Test the precoding matrix generation for eight layers and eight 1D Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_EightLayerEightPorts1D)
{
  // Antenna configuration: N1=4, N2=1, as per TS 38.214 Table 5.2.2.2.1-12.
  static constexpr unsigned N1             = 4;
  static constexpr unsigned O1             = 4;
  static constexpr unsigned nof_beams      = O1 * N1 / 2;
  static constexpr unsigned nof_layers     = 8;
  static constexpr unsigned nof_ports      = 2 * N1;
  static constexpr unsigned nof_pol_shifts = 2;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
      precoding_weight_matrix precoding = make_eight_layer_eight_ports_type1_sp(i_1_1, i_2);

      ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
      ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

      // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-12.
      // Layer 0: beam l = i_1_1, phi = pi/2 * i_2.
      // Layer 1: beam l = i_1_1, phi = pi/2 * i_2 + pi.
      // Layer 2: beam l = i_1_1 + O1, phi = pi/2 * i_2.
      // Layer 3: beam l = i_1_1 + O1, phi = pi/2 * i_2 + pi.
      // Layer 4: beam l = i_1_1 + 2 * O1, phi = 0.
      // Layer 5: beam l = i_1_1 + 2 * O1, phi = pi.
      // Layer 6: beam l = i_1_1 + 3 * O1, phi = 0.
      // Layer 7: beam l = i_1_1 + 3 * O1, phi = pi.
      float    phi           = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
      unsigned layer_beam[8] = {
          i_1_1, i_1_1, i_1_1 + O1, i_1_1 + O1, i_1_1 + 2 * O1, i_1_1 + 2 * O1, i_1_1 + 3 * O1, i_1_1 + 3 * O1};
      float layer_pol[8] = {phi,
                            phi + static_cast<float>(M_PI),
                            phi,
                            phi + static_cast<float>(M_PI),
                            0.0F,
                            static_cast<float>(M_PI),
                            0.0F,
                            static_cast<float>(M_PI)};

      for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
        for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
          unsigned element       = i_port % N1;
          bool     is_second_pol = (i_port >= N1);

          float beam_phase = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam[i_layer] * element) /
                             static_cast<float>(nof_beams);
          float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

          cf_t expected = scaling * std::polar(1.0F, beam_phase + pol_phase);
          ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
        }
      }
    }
  }
}

// Test the precoding matrix generation for one layer and eight 2D Type1 Single-Panel Mode1 antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_OneLayerEightPorts2D)
{
  // Antenna configuration: N1=2, N2=2, O1=O2=4, as per TS 38.214 Table 5.2.2.2.1-5.
  static constexpr unsigned N1             = 2;
  static constexpr unsigned N2             = 2;
  static constexpr unsigned O              = 4;
  static constexpr unsigned nof_beams      = O * N1;
  static constexpr unsigned nof_ports      = 2 * N1 * N2;
  static constexpr unsigned nof_pol_shifts = 4;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_1_2 = 0; i_1_2 != nof_beams; ++i_1_2) {
      for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
        precoding_weight_matrix precoding = make_one_layer_eight_ports_type1_sp_mode1(i_1_1, i_1_2, i_2);

        ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
        ASSERT_EQ(precoding.get_nof_layers(), 1);

        // Layer 0: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2.
        for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
          unsigned element       = i_port % (N1 * N2);
          bool     is_second_pol = (i_port >= N1 * N2);
          unsigned i_h           = element / N2;
          unsigned i_v           = element % N2;

          float beam_phase_h =
              2.0F * static_cast<float>(M_PI) * static_cast<float>(i_1_1 * i_h) / static_cast<float>(nof_beams);
          float beam_phase_v =
              2.0F * static_cast<float>(M_PI) * static_cast<float>(i_1_2 * i_v) / static_cast<float>(nof_beams);
          float pol_phase = is_second_pol ? (static_cast<float>(M_PI_2) * static_cast<float>(i_2)) : 0.0F;

          cf_t expected = scaling * std::polar(1.0F, beam_phase_h + beam_phase_v + pol_phase);
          ASSERT_CF_EQ(precoding.get_coefficient(0, i_port), expected);
        }
      }
    }
  }
}

// Test the precoding matrix generation for two layers and eight 2D Type1 Single-Panel Mode1 antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_TwoLayerEightPorts2D)
{
  // Antenna configuration: N1=2, N2=2, O1=O2=4, as per TS 38.214 Table 5.2.2.2.1-6.
  static constexpr unsigned N1             = 2;
  static constexpr unsigned N2             = 2;
  static constexpr unsigned O              = 4;
  static constexpr unsigned nof_beams      = O * N1;
  static constexpr unsigned nof_layers     = 2;
  static constexpr unsigned nof_ports      = 2 * N1 * N2;
  static constexpr unsigned nof_pol_shifts = 2;
  static constexpr unsigned nof_offsets    = 4;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_1_2 = 0; i_1_2 != nof_beams; ++i_1_2) {
      for (unsigned i_1_3 = 0; i_1_3 != nof_offsets; ++i_1_3) {
        for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
          precoding_weight_matrix precoding = make_two_layer_eight_ports_type1_sp_mode1(i_1_1, i_1_2, i_1_3, i_2);

          ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
          ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

          // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-6.
          // Layer 0: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2.
          // Layer 1: beam_h = i_1_1 + k1, beam_v = i_1_2 + k2, phi = pi/2 * i_2 + pi.
          unsigned k1              = (i_1_3 % 2 == 0) ? 0 : O;
          unsigned k2              = (i_1_3 < 2) ? 0 : O;
          float    phi             = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
          unsigned layer_beam_h[2] = {i_1_1, i_1_1 + k1};
          unsigned layer_beam_v[2] = {i_1_2, i_1_2 + k2};
          float    layer_pol[2]    = {phi, phi + static_cast<float>(M_PI)};

          for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
            for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
              unsigned element       = i_port % (N1 * N2);
              bool     is_second_pol = (i_port >= N1 * N2);
              unsigned i_h           = element / N2;
              unsigned i_v           = element % N2;

              float beam_phase_h = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_h[i_layer] * i_h) /
                                   static_cast<float>(nof_beams);
              float beam_phase_v = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_v[i_layer] * i_v) /
                                   static_cast<float>(nof_beams);
              float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

              cf_t expected = scaling * std::polar(1.0F, beam_phase_h + beam_phase_v + pol_phase);
              ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
            }
          }
        }
      }
    }
  }
}

// Test the precoding matrix generation for three layers and eight 2D Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_ThreeLayerEightPorts2D)
{
  // Antenna configuration: N1=2, N2=2, O1=O2=4, as per TS 38.214 Table 5.2.2.2.1-7.
  static constexpr unsigned N1             = 2;
  static constexpr unsigned N2             = 2;
  static constexpr unsigned O              = 4;
  static constexpr unsigned nof_beams      = O * N1;
  static constexpr unsigned nof_layers     = 3;
  static constexpr unsigned nof_ports      = 2 * N1 * N2;
  static constexpr unsigned nof_pol_shifts = 2;
  static constexpr unsigned nof_offsets    = 3;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_1_2 = 0; i_1_2 != nof_beams; ++i_1_2) {
      for (unsigned i_1_3 = 0; i_1_3 != nof_offsets; ++i_1_3) {
        for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
          precoding_weight_matrix precoding = make_three_layer_eight_ports_type1_sp(i_1_1, i_1_2, i_1_3, i_2);

          ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
          ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

          // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-7.
          // Layer 0: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2.
          // Layer 1: beam_h = i_1_1 + k1, beam_v = i_1_2 + k2, phi = pi/2 * i_2.
          // Layer 2: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2 + pi.
          unsigned k1              = (i_1_3 == 1) ? 0 : O;
          unsigned k2              = (i_1_3 == 0) ? 0 : O;
          float    phi             = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
          unsigned layer_beam_h[3] = {i_1_1, i_1_1 + k1, i_1_1};
          unsigned layer_beam_v[3] = {i_1_2, i_1_2 + k2, i_1_2};
          float    layer_pol[3]    = {phi, phi, phi + static_cast<float>(M_PI)};

          for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
            for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
              unsigned element       = i_port % (N1 * N2);
              bool     is_second_pol = (i_port >= N1 * N2);
              unsigned i_h           = element / N2;
              unsigned i_v           = element % N2;

              float beam_phase_h = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_h[i_layer] * i_h) /
                                   static_cast<float>(nof_beams);
              float beam_phase_v = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_v[i_layer] * i_v) /
                                   static_cast<float>(nof_beams);
              float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

              cf_t expected = scaling * std::polar(1.0F, beam_phase_h + beam_phase_v + pol_phase);
              ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
            }
          }
        }
      }
    }
  }
}

// Test the precoding matrix generation for four layers and eight 2D Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_FourLayerEightPorts2D)
{
  // Antenna configuration: N1=2, N2=2, O1=O2=4, as per TS 38.214 Table 5.2.2.2.1-8.
  static constexpr unsigned N1             = 2;
  static constexpr unsigned N2             = 2;
  static constexpr unsigned O              = 4;
  static constexpr unsigned nof_beams      = O * N1;
  static constexpr unsigned nof_layers     = 4;
  static constexpr unsigned nof_ports      = 2 * N1 * N2;
  static constexpr unsigned nof_pol_shifts = 2;
  static constexpr unsigned nof_offsets    = 3;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_1_2 = 0; i_1_2 != nof_beams; ++i_1_2) {
      for (unsigned i_1_3 = 0; i_1_3 != nof_offsets; ++i_1_3) {
        for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
          precoding_weight_matrix precoding = make_four_layer_eight_ports_type1_sp(i_1_1, i_1_2, i_1_3, i_2);

          ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
          ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

          // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-8.
          // Layer 0: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2.
          // Layer 1: beam_h = i_1_1 + k1, beam_v = i_1_2 + k2, phi = pi/2 * i_2.
          // Layer 2: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2 + pi.
          // Layer 3: beam_h = i_1_1 + k1, beam_v = i_1_2 + k2, phi = pi/2 * i_2 + pi.
          unsigned k1              = (i_1_3 == 1) ? 0 : O;
          unsigned k2              = (i_1_3 == 0) ? 0 : O;
          float    phi             = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
          unsigned layer_beam_h[4] = {i_1_1, i_1_1 + k1, i_1_1, i_1_1 + k1};
          unsigned layer_beam_v[4] = {i_1_2, i_1_2 + k2, i_1_2, i_1_2 + k2};
          float    layer_pol[4]    = {phi, phi, phi + static_cast<float>(M_PI), phi + static_cast<float>(M_PI)};

          for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
            for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
              unsigned element       = i_port % (N1 * N2);
              bool     is_second_pol = (i_port >= N1 * N2);
              unsigned i_h           = element / N2;
              unsigned i_v           = element % N2;

              float beam_phase_h = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_h[i_layer] * i_h) /
                                   static_cast<float>(nof_beams);
              float beam_phase_v = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_v[i_layer] * i_v) /
                                   static_cast<float>(nof_beams);
              float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

              cf_t expected = scaling * std::polar(1.0F, beam_phase_h + beam_phase_v + pol_phase);
              ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
            }
          }
        }
      }
    }
  }
}

// Test the precoding matrix generation for five layers and eight 2D Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_FiveLayerEightPorts2D)
{
  // Antenna configuration: N1=2, N2=2, O1=O2=4, as per TS 38.214 Table 5.2.2.2.1-9.
  static constexpr unsigned N1             = 2;
  static constexpr unsigned N2             = 2;
  static constexpr unsigned O              = 4;
  static constexpr unsigned nof_beams      = O * N1;
  static constexpr unsigned nof_layers     = 5;
  static constexpr unsigned nof_ports      = 2 * N1 * N2;
  static constexpr unsigned nof_pol_shifts = 2;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_1_2 = 0; i_1_2 != nof_beams; ++i_1_2) {
      for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
        precoding_weight_matrix precoding = make_five_layer_eight_ports_type1_sp(i_1_1, i_1_2, i_2);

        ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
        ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

        // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-9.
        // Layer 0: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2.
        // Layer 1: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2 + pi.
        // Layer 2: beam_h = i_1_1 + O, beam_v = i_1_2, phi = 0.
        // Layer 3: beam_h = i_1_1 + O, beam_v = i_1_2, phi = pi.
        // Layer 4: beam_h = i_1_1 + O, beam_v = i_1_2 + O, phi = 0.
        float    phi             = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
        unsigned layer_beam_h[5] = {i_1_1, i_1_1, i_1_1 + O, i_1_1 + O, i_1_1 + O};
        unsigned layer_beam_v[5] = {i_1_2, i_1_2, i_1_2, i_1_2, i_1_2 + O};
        float    layer_pol[5]    = {phi, phi + static_cast<float>(M_PI), 0.0F, static_cast<float>(M_PI), 0.0F};

        for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
          for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
            unsigned element       = i_port % (N1 * N2);
            bool     is_second_pol = (i_port >= N1 * N2);
            unsigned i_h           = element / N2;
            unsigned i_v           = element % N2;

            float beam_phase_h = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_h[i_layer] * i_h) /
                                 static_cast<float>(nof_beams);
            float beam_phase_v = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_v[i_layer] * i_v) /
                                 static_cast<float>(nof_beams);
            float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

            cf_t expected = scaling * std::polar(1.0F, beam_phase_h + beam_phase_v + pol_phase);
            ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
          }
        }
      }
    }
  }
}

// Test the precoding matrix generation for six layers and eight 2D Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_SixLayerEightPorts2D)
{
  // Antenna configuration: N1=2, N2=2, O1=O2=4, as per TS 38.214 Table 5.2.2.2.1-10.
  static constexpr unsigned N1             = 2;
  static constexpr unsigned N2             = 2;
  static constexpr unsigned O              = 4;
  static constexpr unsigned nof_beams      = O * N1;
  static constexpr unsigned nof_layers     = 6;
  static constexpr unsigned nof_ports      = 2 * N1 * N2;
  static constexpr unsigned nof_pol_shifts = 2;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_1_2 = 0; i_1_2 != nof_beams; ++i_1_2) {
      for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
        precoding_weight_matrix precoding = make_six_layer_eight_ports_type1_sp(i_1_1, i_1_2, i_2);

        ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
        ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

        // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-10.
        // Layer 0: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2.
        // Layer 1: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2 + pi.
        // Layer 2: beam_h = i_1_1 + O, beam_v = i_1_2, phi = 0.
        // Layer 3: beam_h = i_1_1 + O, beam_v = i_1_2, phi = pi.
        // Layer 4: beam_h = i_1_1 + O, beam_v = i_1_2 + O, phi = 0.
        // Layer 5: beam_h = i_1_1 + O, beam_v = i_1_2 + O, phi = pi.
        float    phi             = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
        unsigned layer_beam_h[6] = {i_1_1, i_1_1, i_1_1 + O, i_1_1 + O, i_1_1 + O, i_1_1 + O};
        unsigned layer_beam_v[6] = {i_1_2, i_1_2, i_1_2, i_1_2, i_1_2 + O, i_1_2 + O};
        float    layer_pol[6]    = {
            phi, phi + static_cast<float>(M_PI), 0.0F, static_cast<float>(M_PI), 0.0F, static_cast<float>(M_PI)};

        for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
          for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
            unsigned element       = i_port % (N1 * N2);
            bool     is_second_pol = (i_port >= N1 * N2);
            unsigned i_h           = element / N2;
            unsigned i_v           = element % N2;

            float beam_phase_h = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_h[i_layer] * i_h) /
                                 static_cast<float>(nof_beams);
            float beam_phase_v = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_v[i_layer] * i_v) /
                                 static_cast<float>(nof_beams);
            float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

            cf_t expected = scaling * std::polar(1.0F, beam_phase_h + beam_phase_v + pol_phase);
            ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
          }
        }
      }
    }
  }
}

// Test the precoding matrix generation for seven layers and eight 2D Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_SevenLayerEightPorts2D)
{
  // Antenna configuration: N1=2, N2=2, O1=O2=4, as per TS 38.214 Table 5.2.2.2.1-11.
  static constexpr unsigned N1             = 2;
  static constexpr unsigned N2             = 2;
  static constexpr unsigned O              = 4;
  static constexpr unsigned nof_beams      = O * N1;
  static constexpr unsigned nof_layers     = 7;
  static constexpr unsigned nof_ports      = 2 * N1 * N2;
  static constexpr unsigned nof_pol_shifts = 2;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_1_2 = 0; i_1_2 != nof_beams; ++i_1_2) {
      for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
        precoding_weight_matrix precoding = make_seven_layer_eight_ports_type1_sp(i_1_1, i_1_2, i_2);

        ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
        ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

        // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-11.
        // Layer 0: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2.
        // Layer 1: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2 + pi.
        // Layer 2: beam_h = i_1_1 + O, beam_v = i_1_2, phi = pi/2 * i_2.
        // Layer 3: beam_h = i_1_1, beam_v = i_1_2 + O, phi = 0.
        // Layer 4: beam_h = i_1_1, beam_v = i_1_2 + O, phi = pi.
        // Layer 5: beam_h = i_1_1 + O, beam_v = i_1_2 + O, phi = 0.
        // Layer 6: beam_h = i_1_1 + O, beam_v = i_1_2 + O, phi = pi.
        float    phi             = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
        unsigned layer_beam_h[7] = {i_1_1, i_1_1, i_1_1 + O, i_1_1, i_1_1, i_1_1 + O, i_1_1 + O};
        unsigned layer_beam_v[7] = {i_1_2, i_1_2, i_1_2, i_1_2 + O, i_1_2 + O, i_1_2 + O, i_1_2 + O};
        float    layer_pol[7]    = {
            phi, phi + static_cast<float>(M_PI), phi, 0.0F, static_cast<float>(M_PI), 0.0F, static_cast<float>(M_PI)};

        for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
          for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
            unsigned element       = i_port % (N1 * N2);
            bool     is_second_pol = (i_port >= N1 * N2);
            unsigned i_h           = element / N2;
            unsigned i_v           = element % N2;

            float beam_phase_h = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_h[i_layer] * i_h) /
                                 static_cast<float>(nof_beams);
            float beam_phase_v = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_v[i_layer] * i_v) /
                                 static_cast<float>(nof_beams);
            float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

            cf_t expected = scaling * std::polar(1.0F, beam_phase_h + beam_phase_v + pol_phase);
            ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
          }
        }
      }
    }
  }
}

// Test the precoding matrix generation for eight layers and eight 2D Type1 Single-Panel antenna ports.
TEST(precoding_test, Type1SinglePanelMode1_EightLayerEightPorts2D)
{
  // Antenna configuration: N1=2, N2=2, O1=O2=4, as per TS 38.214 Table 5.2.2.2.1-12.
  static constexpr unsigned N1             = 2;
  static constexpr unsigned N2             = 2;
  static constexpr unsigned O              = 4;
  static constexpr unsigned nof_beams      = O * N1;
  static constexpr unsigned nof_layers     = 8;
  static constexpr unsigned nof_ports      = 2 * N1 * N2;
  static constexpr unsigned nof_pol_shifts = 2;
  static const float        scaling        = 1.0F / std::sqrt(static_cast<float>(nof_layers * nof_ports));

  for (unsigned i_1_1 = 0; i_1_1 != nof_beams; ++i_1_1) {
    for (unsigned i_1_2 = 0; i_1_2 != nof_beams; ++i_1_2) {
      for (unsigned i_2 = 0; i_2 != nof_pol_shifts; ++i_2) {
        precoding_weight_matrix precoding = make_eight_layer_eight_ports_type1_sp(i_1_1, i_1_2, i_2);

        ASSERT_EQ(precoding.get_nof_ports(), nof_ports);
        ASSERT_EQ(precoding.get_nof_layers(), nof_layers);

        // Per-layer configuration, as per TS 38.214 Table 5.2.2.2.1-12.
        // Layer 0: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2.
        // Layer 1: beam_h = i_1_1, beam_v = i_1_2, phi = pi/2 * i_2 + pi.
        // Layer 2: beam_h = i_1_1 + O, beam_v = i_1_2, phi = pi/2 * i_2.
        // Layer 3: beam_h = i_1_1 + O, beam_v = i_1_2, phi = pi/2 * i_2 + pi.
        // Layer 4: beam_h = i_1_1, beam_v = i_1_2 + O, phi = 0.
        // Layer 5: beam_h = i_1_1, beam_v = i_1_2 + O, phi = pi.
        // Layer 6: beam_h = i_1_1 + O, beam_v = i_1_2 + O, phi = 0.
        // Layer 7: beam_h = i_1_1 + O, beam_v = i_1_2 + O, phi = pi.
        float    phi             = static_cast<float>(M_PI_2) * static_cast<float>(i_2);
        unsigned layer_beam_h[8] = {i_1_1, i_1_1, i_1_1 + O, i_1_1 + O, i_1_1, i_1_1, i_1_1 + O, i_1_1 + O};
        unsigned layer_beam_v[8] = {i_1_2, i_1_2, i_1_2, i_1_2, i_1_2 + O, i_1_2 + O, i_1_2 + O, i_1_2 + O};
        float    layer_pol[8]    = {phi,
                                    phi + static_cast<float>(M_PI),
                                    phi,
                                    phi + static_cast<float>(M_PI),
                                    0.0F,
                                    static_cast<float>(M_PI),
                                    0.0F,
                                    static_cast<float>(M_PI)};

        for (unsigned i_layer = 0; i_layer != nof_layers; ++i_layer) {
          for (unsigned i_port = 0; i_port != nof_ports; ++i_port) {
            unsigned element       = i_port % (N1 * N2);
            bool     is_second_pol = (i_port >= N1 * N2);
            unsigned i_h           = element / N2;
            unsigned i_v           = element % N2;

            float beam_phase_h = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_h[i_layer] * i_h) /
                                 static_cast<float>(nof_beams);
            float beam_phase_v = 2.0F * static_cast<float>(M_PI) * static_cast<float>(layer_beam_v[i_layer] * i_v) /
                                 static_cast<float>(nof_beams);
            float pol_phase = is_second_pol ? layer_pol[i_layer] : 0.0F;

            cf_t expected = scaling * std::polar(1.0F, beam_phase_h + beam_phase_v + pol_phase);
            ASSERT_CF_EQ(precoding.get_coefficient(i_layer, i_port), expected);
          }
        }
      }
    }
  }
}
