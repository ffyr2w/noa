#pragma once

#include "noa/unified/Array.h"

namespace noa::math::fft {
    using Remap = noa::fft::Remap;
    using Norm = noa::fft::Norm;

    /// Standardizes (mean=0, stddev=1) a real-space signal, in Fourier space.
    /// \tparam REMAP       Remapping operator. Should be H2H, HC2HC, F2F or FC2FC.
    /// \tparam T           cfloat_t or cdouble_t.
    /// \param[in] input    Input FFT.
    /// \param[out] output  Output FFT. Can be equal to \p input.
    ///                     The C2R transform of \p output has its mean set to 0 and its stddev set to 1.
    /// \param shape        Rightmost (logical) shape of \p input and \p output.
    /// \param norm         Normalization mode of \p input.
    template<Remap REMAP, typename T, typename = std::enable_if_t<traits::is_complex_v<T>>>
    void standardize(const Array<T>& input, const Array<T>& output, size4_t shape, Norm norm = Norm::NORM_FORWARD);
}

#define NOA_UNIFIED_STANDARDIZE_
#include "noa/unified/math/fft/Standardize.inl"
#undef NOA_UNIFIED_STANDARDIZE_
