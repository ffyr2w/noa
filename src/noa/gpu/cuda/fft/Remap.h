/// \file noa/gpu/cuda/fft/Remap.h
/// \brief Remap FFTs.
/// \author Thomas - ffyr2w
/// \date 19 Jun 2021

#pragma once

#include "noa/common/Definitions.h"
#include "noa/gpu/cuda/Exception.h"
#include "noa/gpu/cuda/Types.h"
#include "noa/gpu/cuda/Stream.h"
#include "noa/gpu/cuda/memory/PtrDevice.h"
#include "noa/gpu/cuda/memory/Copy.h"

namespace noa::cuda::fft::details {
    template<typename T>
    void hc2h(const shared_t<T[]>& input, size4_t input_strides,
              const shared_t<T[]>& output, size4_t output_strides,
              size4_t shape, Stream& stream);

    template<typename T>
    void h2hc(const shared_t<T[]>& input, size4_t input_strides,
              const shared_t<T[]>& output, size4_t output_strides,
              size4_t shape, Stream& stream);

    template<typename T>
    void fc2f(const shared_t<T[]>& input, size4_t input_strides,
              const shared_t<T[]>& output, size4_t output_strides,
              size4_t shape, Stream& stream);

    template<typename T>
    void f2fc(const shared_t<T[]>& input, size4_t input_strides,
              const shared_t<T[]>& output, size4_t output_strides,
              size4_t shape, Stream& stream);

    template<typename T>
    void f2h(const shared_t<T[]>& input, size4_t input_strides,
             const shared_t<T[]>& output, size4_t output_strides,
             size4_t shape, Stream& stream);

    template<typename T>
    void h2f(const shared_t<T[]>& input, size4_t input_strides,
             const shared_t<T[]>& output, size4_t output_strides,
             size4_t shape, Stream& stream);

    template<typename T>
    void f2hc(const shared_t<T[]>& input, size4_t input_strides,
              const shared_t<T[]>& output, size4_t output_strides,
              size4_t shape, Stream& stream);

    template<typename T>
    void hc2f(const shared_t<T[]>& input, size4_t input_strides,
              const shared_t<T[]>& output, size4_t output_strides,
              size4_t shape, Stream& stream);

    template<typename T>
    void fc2h(const shared_t<T[]>& input, size4_t input_strides,
              const shared_t<T[]>& output, size4_t output_strides,
              size4_t shape, Stream& stream);

    template<typename T>
    void fc2hc(const shared_t<T[]>& input, size4_t input_strides,
               const shared_t<T[]>& output, size4_t output_strides,
               size4_t shape, Stream& stream);
}

namespace noa::cuda::fft {
    using Remap = ::noa::fft::Remap;

    /// Remaps FFT(s).
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t or cdouble_t.
    /// \param remap            Remapping operation. See noa::fft::Remap for more details.
    /// \param[in] input        On the \b device. Input FFT to remap.
    /// \param input_strides    BDHW strides, in elements, of \p input.
    /// \param[out] output      On the \b device. Remapped FFT.
    /// \param output_strides   BDHW strides, in elements, of \p output.
    /// \param shape            BDHW shape, in elements.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    ///
    /// \note This function is asynchronous relative to the host and may return before completion.
    /// \note If \p remap is \c H2HC, \p input can be equal to \p output, only iff \p shape[2] is even,
    ///       and \p shape[1] is even or 1.
    template<typename T, typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void remap(Remap remap,
               const shared_t<T[]>& input, size4_t input_strides,
               const shared_t<T[]>& output, size4_t output_strides,
               size4_t shape, Stream& stream) {
        switch (remap) {
            case Remap::H2H:
            case Remap::HC2HC:
                if (input != output)
                    memory::copy(input, input_strides, output, output_strides, shape.fft(), stream);
                break;
            case Remap::F2F:
            case Remap::FC2FC:
                if (input != output)
                    memory::copy(input, input_strides, output, output_strides, shape, stream);
                break;
            case Remap::H2HC:
                return details::h2hc(input, input_strides, output, output_strides, shape, stream);
            case Remap::HC2H:
                return details::hc2h(input, input_strides, output, output_strides, shape, stream);
            case Remap::H2F:
                return details::h2f(input, input_strides, output, output_strides, shape, stream);
            case Remap::F2H:
                return details::f2h(input, input_strides, output, output_strides, shape, stream);
            case Remap::F2FC:
                return details::f2fc(input, input_strides, output, output_strides, shape, stream);
            case Remap::FC2F:
                return details::fc2f(input, input_strides, output, output_strides, shape, stream);
            case Remap::HC2F:
                return details::hc2f(input, input_strides, output, output_strides, shape, stream);
            case Remap::F2HC:
                return details::f2hc(input, input_strides, output, output_strides, shape, stream);
            case Remap::FC2H:
                return details::fc2h(input, input_strides, output, output_strides, shape, stream);
            case Remap::FC2HC:
                return details::fc2hc(input, input_strides, output, output_strides, shape, stream);
            case Remap::H2FC: {
                memory::PtrDevice<T> tmp{shape.elements(), stream};
                details::h2f(input, input_strides, tmp.share(), shape.strides(), shape, stream);
                details::f2fc(tmp.share(), shape.strides(), output, output_strides, shape, stream);
                break;
            }
            case noa::fft::HC2FC: {
                memory::PtrDevice<T> tmp{shape.elements(), stream};
                details::hc2f(input, input_strides, tmp.share(), shape.strides(), shape, stream);
                details::f2fc(tmp.share(), shape.strides(), output, output_strides, shape, stream);
                break;
            }
        }
    }
}
