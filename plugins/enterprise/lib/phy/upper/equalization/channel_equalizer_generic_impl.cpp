/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "channel_equalizer_generic_impl.h"
#include "equalize_mmse_mxn_simd.h"
#include "equalize_zf_mxn_simd.h"
#include "interleave_layers.h"
#include "ocudu/adt/interval.h"
#include "ocudu/ocuduvec/conversion.h"
#include "ocudu/ocuduvec/copy.h"
#include "ocudu/ocuduvec/simd.h"
#include "ocudu/ocuduvec/zero.h"
#include "ocudu/support/math/math_utils.h"

using namespace ocudu;

#if OCUDU_SIMD_CF_SIZE

bool channel_equalizer_generic_impl::is_supported(channel_equalizer_algorithm_type algorithm,
                                                  unsigned                         nof_ports,
                                                  unsigned                         nof_layers)
{
  // Only one, two and four ports are currently supported.
  if ((nof_ports != 1) && (nof_ports != 2) && (nof_ports != 4)) {
    return false;
  }

  // The number of layers cannot be greater than the number of ports.
  if (nof_ports < nof_layers) {
    return false;
  }

  // ZF MMSE algorithms support from one to four layers.
  static constexpr interval<unsigned, true> nof_layers_range(1, 4);
  if (!nof_layers_range.contains(nof_layers)) {
    return false;
  }

  // Otherwise it is supported.
  return true;
}

template <unsigned NofLayers, unsigned NofPorts>
void equalize_zf_mxn(span<cf_t>                            eq_symbols,
                     span<float>                           noise_vars,
                     const re_buffer_reader<cbf16_t>&      ch_symbols,
                     const channel_equalizer::ch_est_list& ch_estimates,
                     float                                 noise_var_est,
                     float                                 tx_scaling)
{
  unsigned i_re   = 0;
  unsigned nof_re = ch_symbols.get_nof_re();

  ocudu_assert(eq_symbols.size() == NofLayers * nof_re, "Invalid equalized symbol size.");
  ocudu_assert(noise_vars.size() == NofLayers * nof_re, "Invalid noise variance size.");
  ocudu_assert(ch_symbols.get_nof_slices() == NofPorts,
               "Invalid channel symbols number of ports (i.e., {}).",
               ch_symbols.get_nof_slices());
  ocudu_assert(ch_estimates.get_nof_rx_ports() == NofPorts,
               "Invalid channel estimates number of ports (i.e., {}).",
               ch_estimates.get_nof_rx_ports());
  ocudu_assert(ch_estimates.get_nof_tx_layers() == NofLayers,
               "Invalid channel estimates number of layers (i.e., {}).",
               ch_estimates.get_nof_tx_layers());

  // Skip processing if the noise variance is NaN, infinity or negative.
  if (!std::isnormal(noise_var_est) || (noise_var_est < 0.0F)) {
    ocuduvec::zero(eq_symbols);
    std::fill(noise_vars.begin(), noise_vars.end(), std::numeric_limits<float>::infinity());
    return;
  }

  // Extract views of input data.
  std::array<span<const cbf16_t>, NofPorts>                        ch_symbols_view;
  std::array<std::array<span<const cbf16_t>, NofLayers>, NofPorts> ch_estimates_view;
  for (unsigned i_port = 0; i_port != NofPorts; ++i_port) {
    ch_symbols_view[i_port] = ch_symbols.get_slice(i_port);
    for (unsigned i_layer = 0; i_layer != NofLayers; ++i_layer) {
      ch_estimates_view[i_port][i_layer] = ch_estimates.get_channel(i_port, i_layer);
    }
  }

  // Process entire batches of OCUDU_SIMD_CF_SIZE batches.
  for (unsigned nof_re_end = (nof_re / OCUDU_SIMD_CF_SIZE) * OCUDU_SIMD_CF_SIZE; i_re != nof_re_end;
       i_re += OCUDU_SIMD_CF_SIZE) {
    // Prepare channel matrix.
    simd_cf_t ch_symbols_re[NofPorts];
    simd_cf_t ch_estimates_re[NofPorts][NofLayers];
    for (unsigned i_port = 0; i_port != NofPorts; ++i_port) {
      ch_symbols_re[i_port] = ocudu_simd_cbf16_loadu(&ch_symbols_view[i_port][i_re]);
      for (unsigned i_layer = 0; i_layer != NofLayers; ++i_layer) {
        simd_cf_t ch_estimates_simd = ocudu_simd_cbf16_loadu(&ch_estimates_view[i_port][i_layer][i_re]);
        ch_estimates_simd *= tx_scaling;
        ch_estimates_re[i_port][i_layer] = ch_estimates_simd;
      }
    }

    simd_cf_t eq_symbols_re[NofLayers];
    simd_f_t  noise_vars_re[NofLayers];

    equalize_zf_mxn_simd<NofLayers, NofPorts>(
        eq_symbols_re, noise_vars_re, ch_symbols_re, ch_estimates_re, noise_var_est);

    // Store results.
    interleave_layers_simd<NofLayers>(eq_symbols.subspan(i_re * NofLayers, OCUDU_SIMD_CF_SIZE * NofLayers),
                                      eq_symbols_re);
    interleave_layers_simd<NofLayers>(noise_vars.subspan(i_re * NofLayers, OCUDU_SIMD_F_SIZE * NofLayers),
                                      noise_vars_re);
  }

  // Calculate remainder number RE to process.
  nof_re = nof_re - i_re;

  // Prepare channel matrix.
  simd_cf_t ch_symbols_re[NofPorts];
  simd_cf_t ch_estimates_re[NofPorts][NofLayers];
  for (unsigned i_port = 0; i_port != NofPorts; ++i_port) {
    std::array<cbf16_t, OCUDU_SIMD_CF_SIZE> temp_ch_symbols = {};
    ocuduvec::copy(span<cbf16_t>(temp_ch_symbols).first(nof_re), ch_symbols_view[i_port].last(nof_re));
    ch_symbols_re[i_port] = ocudu_simd_cbf16_loadu(temp_ch_symbols.data());

    for (unsigned i_layer = 0; i_layer != NofLayers; ++i_layer) {
      std::array<cbf16_t, OCUDU_SIMD_CF_SIZE> temp_ch_estimates = {};
      ocuduvec::copy(span<cbf16_t>(temp_ch_estimates).first(nof_re), ch_estimates_view[i_port][i_layer].last(nof_re));

      simd_cf_t ch_estimates_simd = ocudu_simd_cbf16_loadu(temp_ch_estimates.data());
      ch_estimates_simd *= tx_scaling;
      ch_estimates_re[i_port][i_layer] = ch_estimates_simd;
    }
  }

  simd_cf_t eq_symbols_re[NofLayers];
  simd_f_t  noise_vars_re[NofLayers];

  // Actual equalization.
  equalize_zf_mxn_simd<NofLayers, NofPorts>(
      eq_symbols_re, noise_vars_re, ch_symbols_re, ch_estimates_re, noise_var_est);

  // Store results.
  interleave_layers_generic<NofLayers>(eq_symbols.subspan(i_re * NofLayers, nof_re * NofLayers), eq_symbols_re);
  interleave_layers_generic<NofLayers>(noise_vars.subspan(i_re * NofLayers, nof_re * NofLayers), noise_vars_re);
}

template <unsigned NofLayers, unsigned NofPorts>
void equalize_mmse_mxn(span<cf_t>                            eq_symbols,
                       span<float>                           noise_vars,
                       const re_buffer_reader<cbf16_t>&      ch_symbols,
                       const channel_equalizer::ch_est_list& ch_estimates,
                       float                                 noise_var_est,
                       float                                 tx_scaling)
{
  unsigned i_re   = 0;
  unsigned nof_re = ch_symbols.get_nof_re();

  ocudu_assert(eq_symbols.size() == NofLayers * nof_re, "Invalid equalized symbol size.");
  ocudu_assert(noise_vars.size() == NofLayers * nof_re, "Invalid noise variance size.");
  ocudu_assert(ch_symbols.get_nof_slices() == NofPorts,
               "Invalid channel symbols number of ports (i.e., {}).",
               ch_symbols.get_nof_slices());
  ocudu_assert(ch_estimates.get_nof_rx_ports() == NofPorts,
               "Invalid channel estimates number of ports (i.e., {}).",
               ch_estimates.get_nof_rx_ports());
  ocudu_assert(ch_estimates.get_nof_tx_layers() == NofLayers,
               "Invalid channel estimates number of layers (i.e., {}).",
               ch_estimates.get_nof_tx_layers());

  // Skip processing if the noise variance is NaN, infinity or negative.
  if (!std::isnormal(noise_var_est) || (noise_var_est < 0.0F)) {
    ocuduvec::zero(eq_symbols);
    std::fill(noise_vars.begin(), noise_vars.end(), std::numeric_limits<float>::infinity());
    return;
  }

  // Extract views of input data.
  std::array<span<const cbf16_t>, NofPorts>                        ch_symbols_view;
  std::array<std::array<span<const cbf16_t>, NofLayers>, NofPorts> ch_estimates_view;
  for (unsigned i_port = 0; i_port != NofPorts; ++i_port) {
    ch_symbols_view[i_port] = ch_symbols.get_slice(i_port);
    for (unsigned i_layer = 0; i_layer != NofLayers; ++i_layer) {
      ch_estimates_view[i_port][i_layer] = ch_estimates.get_channel(i_port, i_layer);
    }
  }

  // Process entire batches of OCUDU_SIMD_CF_SIZE batches.
  for (unsigned nof_re_end = (nof_re / OCUDU_SIMD_CF_SIZE) * OCUDU_SIMD_CF_SIZE; i_re != nof_re_end;
       i_re += OCUDU_SIMD_CF_SIZE) {
    // Prepare channel matrix.
    simd_cf_t ch_symbols_re[NofPorts];
    simd_cf_t ch_estimates_re[NofPorts][NofLayers];
    for (unsigned i_port = 0; i_port != NofPorts; ++i_port) {
      ch_symbols_re[i_port] = ocudu_simd_cbf16_loadu(&ch_symbols_view[i_port][i_re]);
      for (unsigned i_layer = 0; i_layer != NofLayers; ++i_layer) {
        simd_cf_t ch_estimates_simd = ocudu_simd_cbf16_loadu(&ch_estimates_view[i_port][i_layer][i_re]);
        ch_estimates_simd *= tx_scaling;
        ch_estimates_re[i_port][i_layer] = ch_estimates_simd;
      }
    }

    simd_cf_t eq_symbols_re[NofLayers];
    simd_f_t  noise_vars_re[NofLayers];

    equalize_mmse_mxn_simd<NofLayers, NofPorts>(
        eq_symbols_re, noise_vars_re, ch_symbols_re, ch_estimates_re, noise_var_est);

    // Store results.
    interleave_layers_simd<NofLayers>(eq_symbols.subspan(i_re * NofLayers, OCUDU_SIMD_CF_SIZE * NofLayers),
                                      eq_symbols_re);
    interleave_layers_simd<NofLayers>(noise_vars.subspan(i_re * NofLayers, OCUDU_SIMD_F_SIZE * NofLayers),
                                      noise_vars_re);
  }

  // Calculate remainder number RE to process.
  nof_re = nof_re - i_re;

  // Prepare channel matrix.
  simd_cf_t ch_symbols_re[NofPorts];
  simd_cf_t ch_estimates_re[NofPorts][NofLayers];
  for (unsigned i_port = 0; i_port != NofPorts; ++i_port) {
    std::array<cbf16_t, OCUDU_SIMD_CF_SIZE> temp_ch_symbols = {};
    ocuduvec::copy(span<cbf16_t>(temp_ch_symbols).first(nof_re), ch_symbols_view[i_port].last(nof_re));
    ch_symbols_re[i_port] = ocudu_simd_cbf16_loadu(temp_ch_symbols.data());

    for (unsigned i_layer = 0; i_layer != NofLayers; ++i_layer) {
      std::array<cbf16_t, OCUDU_SIMD_CF_SIZE> temp_ch_estimates = {};
      ocuduvec::copy(span<cbf16_t>(temp_ch_estimates).first(nof_re), ch_estimates_view[i_port][i_layer].last(nof_re));

      simd_cf_t ch_estimates_simd = ocudu_simd_cbf16_loadu(temp_ch_estimates.data());
      ch_estimates_simd *= tx_scaling;
      ch_estimates_re[i_port][i_layer] = ch_estimates_simd;
    }
  }

  simd_cf_t eq_symbols_re[NofLayers];
  simd_f_t  noise_vars_re[NofLayers];

  // Actual equalization.
  equalize_mmse_mxn_simd<NofLayers, NofPorts>(
      eq_symbols_re, noise_vars_re, ch_symbols_re, ch_estimates_re, noise_var_est);

  // Store results.
  interleave_layers_generic<NofLayers>(eq_symbols.subspan(i_re * NofLayers, nof_re * NofLayers), eq_symbols_re);
  interleave_layers_generic<NofLayers>(noise_vars.subspan(i_re * NofLayers, nof_re * NofLayers), noise_vars_re);
}

void channel_equalizer_generic_impl::equalize_zf_3x4(span<cf_t>                            eq_symbols,
                                                     span<float>                           noise_vars,
                                                     const re_buffer_reader<cbf16_t>&      ch_symbols,
                                                     const channel_equalizer::ch_est_list& ch_estimates,
                                                     float                                 noise_var_est,
                                                     float                                 tx_scaling)
{
  equalize_zf_mxn<3, 4>(eq_symbols, noise_vars, ch_symbols, ch_estimates, noise_var_est, tx_scaling);
}

void channel_equalizer_generic_impl::equalize_zf_4x4(span<cf_t>                            eq_symbols,
                                                     span<float>                           noise_vars,
                                                     const re_buffer_reader<cbf16_t>&      ch_symbols,
                                                     const channel_equalizer::ch_est_list& ch_estimates,
                                                     float                                 noise_var_est,
                                                     float                                 tx_scaling)
{
  equalize_zf_mxn<4, 4>(eq_symbols, noise_vars, ch_symbols, ch_estimates, noise_var_est, tx_scaling);
}

void channel_equalizer_generic_impl::equalize_mmse_2x2(span<cf_t>                            eq_symbols,
                                                       span<float>                           noise_vars,
                                                       const re_buffer_reader<cbf16_t>&      ch_symbols,
                                                       const channel_equalizer::ch_est_list& ch_estimates,
                                                       float                                 noise_var_est,
                                                       float                                 tx_scaling)
{
  equalize_mmse_mxn<2, 2>(eq_symbols, noise_vars, ch_symbols, ch_estimates, noise_var_est, tx_scaling);
}

void channel_equalizer_generic_impl::equalize_mmse_2x4(span<cf_t>                            eq_symbols,
                                                       span<float>                           noise_vars,
                                                       const re_buffer_reader<cbf16_t>&      ch_symbols,
                                                       const channel_equalizer::ch_est_list& ch_estimates,
                                                       float                                 noise_var_est,
                                                       float                                 tx_scaling)
{
  equalize_mmse_mxn<2, 4>(eq_symbols, noise_vars, ch_symbols, ch_estimates, noise_var_est, tx_scaling);
}

void channel_equalizer_generic_impl::equalize_mmse_3x4(span<cf_t>                            eq_symbols,
                                                       span<float>                           noise_vars,
                                                       const re_buffer_reader<cbf16_t>&      ch_symbols,
                                                       const channel_equalizer::ch_est_list& ch_estimates,
                                                       float                                 noise_var_est,
                                                       float                                 tx_scaling)
{
  equalize_mmse_mxn<3, 4>(eq_symbols, noise_vars, ch_symbols, ch_estimates, noise_var_est, tx_scaling);
}

void channel_equalizer_generic_impl::equalize_mmse_4x4(span<cf_t>                            eq_symbols,
                                                       span<float>                           noise_vars,
                                                       const re_buffer_reader<cbf16_t>&      ch_symbols,
                                                       const channel_equalizer::ch_est_list& ch_estimates,
                                                       float                                 noise_var_est,
                                                       float                                 tx_scaling)
{
  equalize_mmse_mxn<4, 4>(eq_symbols, noise_vars, ch_symbols, ch_estimates, noise_var_est, tx_scaling);
}

#endif // OCUDU_SIMD_CF_SIZE
