#pragma once

#include "noa/core/Types.hpp"
#include "noa/core/signal/fft/CTF.hpp"
#include "noa/algorithms/signal/CTF.hpp"

namespace noa::cpu::signal::fft::details {
    using Remap = ::noa::fft::Remap;

    template<Remap REMAP, bool IS_ANISOTROPIC, typename Input, typename Output, typename CTF>
    constexpr bool is_valid_ctf_v =
            ((IS_ANISOTROPIC && noa::algorithm::signal::fft::is_valid_aniso_ctf_v<CTF>) ||
             (!IS_ANISOTROPIC && noa::algorithm::signal::fft::is_valid_iso_ctf_v<CTF>)) &&
            (REMAP == Remap::H2H || REMAP == Remap::HC2H || REMAP == Remap::H2HC || REMAP == Remap::HC2HC ||
             REMAP == Remap::F2F || REMAP == Remap::FC2F || REMAP == Remap::F2FC || REMAP == Remap::FC2FC) &&
            (noa::traits::are_same_value_type_v<Input, Output> &&
             ((noa::traits::are_all_same_v<Input, Output> &&
               noa::traits::are_real_or_complex_v<Input, Output>) ||
              (noa::traits::is_complex_v<Input> &&
               noa::traits::is_real_v<Output>)));

    template<Remap REMAP, bool IS_ANISOTROPIC, typename Output, typename CTF>
    constexpr bool is_valid_ctf_range_v =
            ((IS_ANISOTROPIC && noa::algorithm::signal::fft::is_valid_aniso_ctf_v<CTF>) ||
             (!IS_ANISOTROPIC && noa::algorithm::signal::fft::is_valid_iso_ctf_v<CTF>)) &&
            (REMAP == Remap::H2H || REMAP == Remap::HC2HC || REMAP == Remap::F2F || REMAP == Remap::FC2FC) &&
            noa::traits::is_any_v<Output, f32, f64>;
}

namespace noa::cpu::signal::fft {
    template<noa::fft::Remap REMAP, typename Input, typename Output, typename CTFIsotropic,
             typename = std::enable_if_t<details::is_valid_ctf_v<REMAP, false, Input, Output, CTFIsotropic>>>
    void ctf_isotropic(
            const Input* input, Strides4<i64> input_strides,
            Output* output, Strides4<i64> output_strides, Shape4<i64> shape,
            const CTFIsotropic& ctf, bool ctf_abs, bool ctf_square, i64 threads);

    template<noa::fft::Remap REMAP, typename Output, typename CTFIsotropic,
             typename = std::enable_if_t<details::is_valid_ctf_range_v<REMAP, false, Output, CTFIsotropic>>>
    void ctf_isotropic(
            Output* output, Strides4<i64> output_strides, Shape4<i64> shape,
            const CTFIsotropic& ctf, bool ctf_abs, bool ctf_square,
            const Vec2<f32>& fftfreq_range, bool fftfreq_range_endpoint, i64 threads);

    template<noa::fft::Remap REMAP, typename Input, typename Output, typename CTFAnisotropic,
             typename = std::enable_if_t<details::is_valid_ctf_v<REMAP, true, Input, Output, CTFAnisotropic>>>
    void ctf_anisotropic(
            const Input* input, const Strides4<i64>& input_strides,
            Output* output, const Strides4<i64>& output_strides, const Shape4<i64>& shape,
            const CTFAnisotropic& ctf, bool ctf_abs, bool ctf_square, i64 threads);

    template<noa::fft::Remap REMAP, typename Output, typename CTFAnisotropic,
             typename = std::enable_if_t<details::is_valid_ctf_range_v<REMAP, true, Output, CTFAnisotropic>>>
    void ctf_anisotropic(
            Output* output, const Strides4<i64>& output_strides, const Shape4<i64>& shape,
            const CTFAnisotropic& ctf, bool ctf_abs, bool ctf_square,
            const Vec2<f32>& fftfreq_range, bool fftfreq_range_endpoint, i64 threads);
}
