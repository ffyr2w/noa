/// \file noa/cpu/fft/Resize.h
/// \brief Fourier crop and pad.
/// \author Thomas - ffyr2w
/// \date 18 Jun 2021

#pragma once

#include "noa/common/Definitions.h"
#include "noa/common/Types.h"
#include "noa/cpu/Stream.h"

namespace noa::cpu::fft::details {
    using Remap = ::noa::fft::Remap;
    template<Remap REMAP, typename T>
    constexpr bool is_valid_resize = (traits::is_float_v<T> || traits::is_complex_v<T>) &&
                                     (REMAP == Remap::H2H || REMAP == Remap::F2F);

    template<typename T>
    void cropH2H(const T* input, size4_t input_strides, size4_t input_shape,
                 T* output, size4_t output_strides, size4_t output_shape);
    template<typename T>
    void cropF2F(const T* input, size4_t input_strides, size4_t input_shape,
                 T* output, size4_t output_strides, size4_t output_shape);
    template<typename T>
    void padH2H(const T* input, size4_t input_strides, size4_t input_shape,
                T* output, size4_t output_strides, size4_t output_shape);
    template<typename T>
    void padF2F(const T* input, size4_t input_strides, size4_t input_shape,
                T* output, size4_t output_strides, size4_t output_shape);
}

namespace noa::cpu::fft {
    using Remap = ::noa::fft::Remap;

    /// Crops or zero-pads a FFT.
    /// \tparam REMAP           FFT Remap. Only H2H and F2F are currently supported.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        On the \b host. FFT to resize.
    /// \param input_strides    BDHW strides of \p input.
    /// \param input_shape      BDHW shape of \p input.
    /// \param[out] output      On the \b host. Resized FFT.
    /// \param output_strides   BDHW strides of \p output.
    /// \param output_shape     BDHW shape of \p output.
    ///                         All dimensions should either be <= or >= than \p input_shape.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \note The batch dimension cannot be resized, i.e. \p input_shape[0] == \p output_shape[0].
    template<Remap REMAP, typename T, typename = std::enable_if_t<details::is_valid_resize<REMAP, T>>>
    NOA_IH void resize(const shared_t<T[]>& input, size4_t input_strides, size4_t input_shape,
                       const shared_t<T[]>& output, size4_t output_strides, size4_t output_shape, Stream& stream) {
        if (all(input_shape >= output_shape)) {
            if constexpr (REMAP == Remap::H2H) {
                stream.enqueue([=]() {
                    details::cropH2H<T>(input.get(), input_strides, input_shape,
                                        output.get(), output_strides, output_shape);
                });
            } else if constexpr (REMAP == Remap::F2F) {
                stream.enqueue([=]() {
                    details::cropF2F<T>(input.get(), input_strides, input_shape,
                                        output.get(), output_strides, output_shape);
                });
            } else {
                static_assert(noa::traits::always_false_v<T>);
            }
        } else {
            if constexpr (REMAP == Remap::H2H) {
                stream.enqueue([=]() {
                    details::padH2H<T>(input.get(), input_strides, input_shape,
                                       output.get(), output_strides, output_shape);
                });
            } else if constexpr (REMAP == Remap::F2F) {
                stream.enqueue([=]() {
                    details::padF2F<T>(input.get(), input_strides, input_shape,
                                       output.get(), output_strides, output_shape);
                });
            } else {
                static_assert(noa::traits::always_false_v<T>);
            }
        }
    }
}
