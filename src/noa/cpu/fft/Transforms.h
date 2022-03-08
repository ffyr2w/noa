/// \file noa/cpu/fft/Transforms.h
/// \brief Fast Fourier Transforms.
/// \author Thomas - ffyr2w
/// \date 18 Jun 2021

#pragma once

#include <fftw3.h>

#include "noa/common/Definitions.h"
#include "noa/common/Profiler.h"
#include "noa/common/Types.h"
#include "noa/cpu/Stream.h"
#include "noa/cpu/fft/Plan.h"

// -- Execute -- //
namespace noa::cpu::fft {
    /// Executes the \p plan.
    /// \param[in,out] stream Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \note It is safe to execute the same plan in parallel by multiple threads. However, since a given plan operates
    ///       by default on a fixed array, one needs to use one of the new-array functions so that different threads
    ///       compute the transform on different data.
    template<typename T>
    NOA_IH void execute(const Plan<T>& plan, Stream& stream) {
        stream.enqueue([&]() {
            NOA_PROFILE_FUNCTION();
            if constexpr (std::is_same_v<T, float>)
                fftwf_execute(plan.get());
            else
                fftw_execute(plan.get());
        });
    }
}

// -- New-array transforms -- //
namespace noa::cpu::fft {
    /// Computes the forward R2C transform encoded in \p plan.
    /// \tparam T               float, double.
    /// \param[in] input        On the \b host. Should match the input used to create \p plan.
    /// \param[out] output      On the \b host. Should match the output used to create \p plan.
    /// \param[in] plan         Existing plan.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    ///
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \note This function is thread-safe as long as \p input and \p output are only accessed by
    ///       one single thread. However \p plan can be access by multiple threads concurrently.
    /// \note The arrays used to create \p plan should be similar to \p input and \p output.
    ///       The shape should be the same. The input and output arrays are the same (in-place) or different
    ///       (out-of-place) if the plan was originally created to be in-place or out-of-place, respectively.
    ///       The alignment should be the same as well.
    template<typename T>
    NOA_IH void r2c(T* input, Complex<T>* output, const Plan<T>& plan, Stream& stream) {
        auto ptr = plan.get();
        stream.enqueue([=]() {
            NOA_PROFILE_FUNCTION();
            if constexpr (std::is_same_v<T, float>)
                fftwf_execute_dft_r2c(ptr, input, reinterpret_cast<fftwf_complex*>(output));
            else
                fftw_execute_dft_r2c(ptr, input, reinterpret_cast<fftw_complex*>(output));
        });
    }

    /// Computes the inverse C2R transform encoded in \p plan.
    /// \tparam T               float, double.
    /// \param[in] input        On the \b host. Should match the input used to create \p plan.
    /// \param[out] output      On the \b host. Should match the output used to create \p plan.
    /// \param[in] plan         Existing plan.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    ///
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \note This function is thread-safe as long as \p input and \p output are only accessed by
    ///       one single thread. However \p plan can be access by multiple threads concurrently.
    /// \note The arrays used to create \p plan should be similar to \p input and \p output.
    ///       The shape should be the same. The input and output arrays are the same (in-place) or different
    ///       (out-of-place) if the plan was originally created to be in-place or out-of-place, respectively.
    ///       The alignment should be the same as well.
    template<typename T>
    NOA_IH void c2r(Complex<T>* input, T* output, const Plan<T>& plan, Stream& stream) {
        auto ptr = plan.get();
        stream.enqueue([=]() {
            NOA_PROFILE_FUNCTION();
            if constexpr (std::is_same_v<T, float>)
                fftwf_execute_dft_c2r(ptr, reinterpret_cast<fftwf_complex*>(input), output);
            else
                fftw_execute_dft_c2r(ptr, reinterpret_cast<fftw_complex*>(input), output);
        });
    }

    /// Computes the C2C transform encoded in \p plan.
    /// \tparam T               float, double.
    /// \param[in] input        On the \b host. Should match the input used to create \p plan.
    /// \param[out] output      On the \b host. Should match the output used to create \p plan.
    /// \param[in] plan         Existing plan.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    ///
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \note This function is thread-safe as long as \p input and \p output are only accessed by
    ///       one single thread. However \p plan can be access by multiple threads concurrently.
    /// \note The arrays used to create \p plan should be similar to \p input and \p output.
    ///       The shape should be the same. The input and output arrays are the same (in-place) or different
    ///       (out-of-place) if the plan was originally created to be in-place or out-of-place, respectively.
    ///       The alignment should be the same as well.
    template<typename T>
    NOA_IH void c2c(Complex<T>* input, Complex<T>* output, const Plan<T>& plan, Stream& stream) {
        auto ptr = plan.get();
        stream.enqueue([=]() {
            NOA_PROFILE_FUNCTION();
            if constexpr (std::is_same_v<T, float>) {
                fftwf_execute_dft(ptr,
                                  reinterpret_cast<fftwf_complex*>(input),
                                  reinterpret_cast<fftwf_complex*>(output));
            } else {
                fftw_execute_dft(ptr,
                                 reinterpret_cast<fftw_complex*>(input),
                                 reinterpret_cast<fftw_complex*>(output));
            }
        });
    }
}

// -- "One time" transforms -- //
namespace noa::cpu::fft {
    /// Computes the forward R2C transform.
    /// \tparam T               float, double.
    /// \param[in] input        On the \b host. Real space array.
    /// \param[out] output      On the \b host. Non-redundant non-centered FFT(s). Can be equal to \p input.
    /// \param shape            Rightmost shape of \p input and \p output.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \see fft::Plan for more details.
    template<typename T>
    NOA_IH void r2c(T* input, Complex<T>* output, size4_t shape, Stream& stream) {
        stream.enqueue([&]() {
            NOA_PROFILE_FUNCTION();
            Plan fast_plan(input, output, shape, fft::ESTIMATE, stream);
            execute(fast_plan, stream);
        });
    }

    /// Computes the forward R2C transform.
    /// \tparam T               float, double.
    /// \param[in] input        On the \b host. Real space array.
    /// \param input_stride     Rightmost strides, in real elements, of \p input.
    /// \param[out] output      On the \b host. Non-redundant non-centered FFT(s). Can be equal to \p input.
    /// \param output_stride    Rightmost strides, in complex elements, of \p output.
    /// \param shape            Rightmost shape of \p input and \p output.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \see fft::Plan for more details.
    template<typename T>
    NOA_IH void r2c(T* input, size4_t input_stride,
                    Complex<T>* output, size4_t output_stride,
                    size4_t shape, Stream& stream) {
        stream.enqueue([&]() {
            NOA_PROFILE_FUNCTION();
            Plan fast_plan(input, input_stride, output, output_stride, shape, fft::ESTIMATE, stream);
            execute(fast_plan, stream);
        });
    }

    /// Computes the in-place R2C transform.
    /// \tparam T               float, double.
    /// \param[in] data         On the \b host. Input should be the real space array with appropriate padding.
    /// \param shape            Rightmost shape of \p data.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \see fft::Plan for more details.
    template<typename T>
    NOA_IH void r2c(T* data, size4_t shape, Stream& stream) {
        r2c(data, reinterpret_cast<Complex<T>*>(data), shape, stream);
    }

    /// Computes the in-place R2C transform.
    /// \tparam T               float, double.
    /// \param[in] data         On the \b host. Input should be the real space array with appropriate padding.
    /// \param stride           Rightmost strides, in real elements, of \p data.
    /// \param shape            Rightmost shape of \p data.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \note Since the transform is in-place, it must be able to hold the complex non-redundant transform.
    ///       As such, the innermost dimension must have the appropriate padding. See fft::Plan for more details
    template<typename T>
    NOA_IH void r2c(T* data, size4_t stride, size4_t shape, Stream& stream) {
        // Since it is in-place, the pitch (in real elements) in the innermost dimension should be:
        //  1: even, since complex elements take 2 real elements
        //  2: have at least 1 (if odd) or 2 (if even) extract real element in the innermost dimension
        NOA_ASSERT(!(stride.pitches()[2] % 2));
        NOA_ASSERT(stride.pitches()[2] >= shape[3] + 1 + size_t(!(shape[3] % 2)));

        const size4_t complex_stride{stride[0] / 2, stride[1] / 2, stride[2] / 2, stride[3]};
        r2c(data, stride, reinterpret_cast<Complex<T>*>(data), complex_stride, shape, stream);
    }

    /// Computes the backward C2R transform.
    /// \tparam T               float, double.
    /// \param[in] input        On the \b host. Non-redundant non-centered FFT(s).
    /// \param[out] output      On the \b host. Real space array. Can be equal to \p input.
    /// \param shape            Rightmost shape of \p input and \p output.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \see fft::Plan for more details.
    template<typename T>
    NOA_IH void c2r(Complex<T>* input, T* output, size4_t shape, Stream& stream) {
        stream.enqueue([&]() {
            NOA_PROFILE_FUNCTION();
            Plan fast_plan(input, output, shape, fft::ESTIMATE, stream);
            execute(fast_plan, stream);
        });
    }

    /// Computes the backward C2R transform.
    /// \tparam T               float, double.
    /// \param[in] input        On the \b host. Non-redundant non-centered FFT(s).
    /// \param input_stride     Rightmost strides, in complex elements, of \p input.
    /// \param[out] output      On the \b host. Real space array. Can be equal to \p input.
    /// \param output_stride    Rightmost strides, in real elements, of \p output.
    /// \param shape            Rightmost shape of \p input and \p output.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \see fft::Plan for more details.
    template<typename T>
    NOA_IH void c2r(Complex<T>* input, size4_t input_stride, T* output, size4_t output_stride,
                    size4_t shape, Stream& stream) {
        stream.enqueue([&]() {
            NOA_PROFILE_FUNCTION();
            Plan fast_plan(input, input_stride, output, output_stride, shape, fft::ESTIMATE, stream);
            execute(fast_plan, stream);
        });
    }

    /// Computes the in-place C2R transform.
    /// \tparam T               float, double.
    /// \param[in] data         On the \b host. Input should be the non-redundant non-centered FFT(s).
    /// \param shape            Rightmost shape of \p data.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \see fft::Plan for more details.
    template<typename T>
    NOA_IH void c2r(Complex<T>* data, size4_t shape, Stream& stream) {
        c2r(data, reinterpret_cast<T*>(data), shape, stream);
    }

    /// Computes the in-place C2R transform.
    /// \tparam T               float, double.
    /// \param[in] data         On the \b host. Input should be the non-redundant non-centered FFT(s).
    /// \param stride           Rightmost strides, in complex elements, of \p data.
    /// \param shape            Rightmost shape of \p data.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \see fft::Plan for more details.
    template<typename T>
    NOA_IH void c2r(Complex<T>* data, size4_t stride, size4_t shape, Stream& stream) {
        const size4_t real_stride{stride[0] * 2, stride[1] * 2, stride[2] * 2, stride[3]};
        c2r(data, stride, reinterpret_cast<T*>(data), real_stride, shape, stream);
    }

    /// Computes the C2C transform.
    /// \tparam T               float, double.
    /// \param[in] input        On the \b host. Input complex data.
    /// \param[out] output      On the \b host. Non-centered FFT(s). Can be equal to \p input.
    /// \param shape            Rightmost shape of \p input and \p output.
    /// \param sign             Sign of the exponent in the formula that defines the Fourier transform.
    ///                         It can be −1 (\c fft::FORWARD) or +1 (\c fft::BACKWARD).
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \see fft::Plan for more details.
    template<typename T>
    NOA_IH void c2c(Complex<T>* input, Complex<T>* output, size4_t shape, Sign sign, Stream& stream) {
        stream.enqueue([&]() {
            NOA_PROFILE_FUNCTION();
            Plan fast_plan(input, output, shape, sign, fft::ESTIMATE, stream);
            execute(fast_plan, stream);
        });
    }

    /// Computes the C2C transform.
    /// \tparam T               float, double.
    /// \param[in] input        On the \b host. Input complex data.
    /// \param input_stride     Rightmost strides, in complex elements, of \p input.
    /// \param[out] output      On the \b host. Non-centered FFT(s). Can be equal to \p input.
    /// \param output_stride    Rightmost strides, in complex elements, of \p output.
    /// \param shape            Rightmost shape of \p input and \p output.
    /// \param sign             Sign of the exponent in the formula that defines the Fourier transform.
    ///                         It can be −1 (\c fft::FORWARD) or +1 (\c fft::BACKWARD).
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \see fft::Plan for more details.
    template<typename T>
    NOA_IH void c2c(Complex<T>* input, size4_t input_stride, Complex<T>* output, size4_t output_stride,
                    size4_t shape, Sign sign, Stream& stream) {
        stream.enqueue([&]() {
            NOA_PROFILE_FUNCTION();
            Plan fast_plan(input, input_stride, output, output_stride, shape, sign, fft::ESTIMATE, stream);
            execute(fast_plan, stream);
        });
    }

    /// Computes the in-place C2C transform.
    /// \tparam T               float, double.
    /// \param[in] data         On the \b host.
    /// \param shape            Rightmost shape of \p data.
    /// \param sign             Sign of the exponent in the formula that defines the Fourier transform.
    ///                         It can be −1 (\c fft::FORWARD) or +1 (\c fft::BACKWARD).
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \see fft::Plan for more details.
    template<typename T>
    NOA_IH void c2c(Complex<T>* data, size4_t shape, Sign sign, Stream& stream) {
        c2c(data, data, shape, sign, stream);
    }

    /// Computes the in-place C2C transform.
    /// \tparam T               float, double.
    /// \param[in] data         On the \b host.
    /// \param stride           Rightmost strides, in complex elements, of \p data.
    /// \param shape            Rightmost shape of \p data.
    /// \param sign             Sign of the exponent in the formula that defines the Fourier transform.
    ///                         It can be −1 (\c fft::FORWARD) or +1 (\c fft::BACKWARD).
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \see fft::Plan for more details.
    template<typename T>
    NOA_IH void c2c(Complex<T>* data, size4_t stride, size4_t shape, Sign sign, Stream& stream) {
        c2c(data, stride, data, stride, shape, sign, stream);
    }
}
