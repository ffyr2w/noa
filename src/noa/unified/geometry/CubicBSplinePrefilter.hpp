#pragma once

#include "noa/cpu/geometry/Prefilter.hpp"
#ifdef NOA_ENABLE_CUDA
#include "noa/gpu/cuda/geometry/Prefilter.cuh"
#endif
#include "noa/unified/Array.hpp"
#include "noa/unified/Utilities.hpp"

namespace noa::geometry {
    /// Applies a prefilter to \p input so that the cubic B-spline values will pass through the sample data.
    /// \details Without prefiltering, cubic B-spline filtering results in smoothened images. This is because the
    ///          cubic B-spline filtering yields a function that does not pass through its coefficients. To end up
    ///          with a cubic B-spline interpolated image that passes through the original samples, we need to
    ///          pre-filter the input.
    /// \param[in] input    Input array of f32, f64, c32, or c64.
    /// \param[out] output  Output array. Can be equal to \p input.
    ///
    /// \see http://www2.cs.uregina.ca/~anima/408/Notes/Interpolation/UniformBSpline.htm
    /// \see http://www.dannyruijters.nl/cubicinterpolation/ for more details.
    template<nt::readable_varray_decay_of_almost_any<f32, f64, c32, c64> Input,
             nt::writable_varray_decay_of_any<nt::mutable_value_type_t<Input>> Output>
    void cubic_bspline_prefilter(Input&& input, Output&& output) {
        check(not input.is_empty() and not output.is_empty(), "Empty array detected");

        const Device device = output.device();
        check(device == input.device(),
              "The input and output arrays must be on the same device, but got input:device={}, output:device={}",
              input.device(), device);

        auto input_strides = ng::broadcast_strides(input, output);
        Stream& stream = Stream::current(device);
        if (device.is_cpu()) {
            auto& cpu_stream = stream.cpu();
            const auto threads = cpu_stream.thread_limit();
            cpu_stream.enqueue([=,
                i = std::forward<Input>(input),
                o = std::forward<Output>(output)
            ] {
                noa::cpu::geometry::cubic_bspline_prefilter(
                    i.get(), input_strides,
                    o.get(), o.strides(),
                    o.shape(), threads);
            });
        } else {
            #ifdef NOA_ENABLE_CUDA
            auto& cuda_stream = stream.cuda();
            noa::cuda::geometry::cubic_bspline_prefilter(
                input.get(), input_strides,
                output.get(), output.strides(),
                output.shape(), cuda_stream);
            cuda_stream.enqueue_attach(
                std::forward<Input>(input),
                std::forward<Output>(output));
            #else
            panic_no_gpu_backend();
            #endif
        }
    }
}
