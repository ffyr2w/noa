#include "noa/gpu/cuda/math/fft/Standardize.h"
#include "noa/gpu/cuda/math/Ewise.h"
#include "noa/gpu/cuda/math/Reduce.h"
#include "noa/gpu/cuda/memory/Set.h"
#include "noa/gpu/cuda/memory/PtrDevice.h"

namespace {
    using namespace ::noa;
    using Norm = cuda::math::fft::Norm;

    template<fft::Remap REMAP, typename T>
    void standardizeFull_(const shared_t<Complex<T>[]>& input, size4_t input_stride,
                          const shared_t<Complex<T>[]>& output, size4_t output_stride,
                          const shared_t<T[]>& buffer, size4_t buffer_stride,
                          size4_t shape, Norm norm, cuda::Stream& stream) {
        NOA_ASSERT(shape.ndim() <= 3);
        const auto count = static_cast<T>(math::prod(shape));
        const auto scale = norm == fft::NORM_FORWARD ? 1 : norm == fft::NORM_ORTHO ? math::sqrt(count) : count;

        size4_t index_dc{};
        if constexpr (REMAP == fft::Remap::FC2FC) {
            index_dc = {0,
                        math::FFTShift(size_t{0}, shape[1]),
                        math::FFTShift(size_t{0}, shape[2]),
                        math::FFTShift(size_t{0}, shape[3])};
        }

        cuda::math::ewise(input, input_stride, buffer, buffer_stride, shape, math::abs_squared_t{}, stream);
        cuda::memory::set(buffer.get() + indexing::at(index_dc, buffer_stride), 1, T{0}, stream);

        const T factor = math::sqrt(cuda::math::sum(buffer, buffer_stride, shape, stream)) / scale;
        cuda::math::ewise(input, input_stride, 1 / factor, output, output_stride, shape, math::multiply_t{}, stream);
        cuda::memory::set(shared_t<Complex<T>[]>{output, output.get() + indexing::at(index_dc, output_stride)},
                          1, Complex<T>{0}, stream);
    }

    template<fft::Remap REMAP, typename T>
    void standardizeHalf_(const shared_t<Complex<T>[]>& input, size4_t input_stride,
                          const shared_t<Complex<T>[]>& output, size4_t output_stride,
                          const shared_t<T[]>& buffer,
                          size4_t shape, size4_t shape_fft, Norm norm, cuda::Stream& stream) {
        NOA_ASSERT(shape.ndim() <= 3);
        using namespace noa::indexing;
        const auto count = static_cast<T>(math::prod(shape));
        const auto scale = norm == fft::NORM_FORWARD ? 1 : norm == fft::NORM_ORTHO ? math::sqrt(count) : count;

        const Subregion original{shape_fft, input_stride};
        const bool even = !(shape[3] % 2);
        size4_t index_dc{};
        if constexpr (REMAP == fft::Remap::HC2HC)
            index_dc = {0, math::FFTShift(size_t{0}, shape[1]), math::FFTShift(size_t{0}, shape[2]), 0};

        // Reduce unique chunk:
        auto subregion = original(ellipsis_t{}, slice_t{1, original.shape()[3] - even});
        auto subregion_ptr = shared_t<Complex<T>[]>{input, input.get() + subregion.offset()};
        auto buffer_stride = subregion.shape().stride();
        cuda::math::ewise(subregion_ptr, subregion.stride(),
                          buffer, buffer_stride,
                          subregion.shape(), math::abs_squared_t{}, stream);
        T factor = 2 * cuda::math::sum(buffer, buffer_stride, subregion.shape(), stream);

        // Reduce common column/plane containing the DC:
        subregion = original(ellipsis_t{}, 0);
        subregion_ptr = shared_t<Complex<T>[]>{input, input.get() + subregion.offset()};
        buffer_stride = subregion.shape().stride();
        cuda::math::ewise(subregion_ptr, subregion.stride(),
                          buffer, buffer_stride,
                          subregion.shape(), math::abs_squared_t{}, stream);
        cuda::memory::set(buffer.get() + indexing::at(index_dc, buffer_stride), 1, T{0}, stream);
        factor += cuda::math::sum(buffer, buffer_stride, subregion.shape(), stream);

        if (even) {
            // Reduce common column/plane containing the real Nyquist:
            subregion = original(ellipsis_t{}, -1);
            subregion_ptr = shared_t<Complex<T>[]>{input, input.get() + subregion.offset()};
            buffer_stride = subregion.shape().stride();
            cuda::math::ewise(subregion_ptr, subregion.stride(),
                              buffer, buffer_stride,
                              subregion.shape(), math::abs_squared_t{}, stream);
            factor += cuda::math::sum(buffer, buffer_stride, subregion.shape(), stream);
        }

        factor = math::sqrt(factor) / scale;
        cuda::math::ewise(input, input_stride, 1 / factor, output, output_stride, shape_fft, math::multiply_t{}, stream);
        cuda::memory::set(shared_t<Complex<T>[]>{output, output.get() + indexing::at(index_dc, output_stride)},
                          1, Complex<T>{0}, stream);
    }
}

namespace noa::cuda::math::fft {
    template<Remap REMAP, typename T>
    void standardize(const shared_t<T[]>& input, size4_t input_stride,
                     const shared_t<T[]>& output, size4_t output_stride,
                     size4_t shape, Norm norm, Stream& stream) {
        using real_t = noa::traits::value_type_t<T>;
        const size4_t shape_{1, shape[1], shape[2], shape[3]};
        const size4_t shape_fft = REMAP == Remap::F2F || REMAP == Remap::FC2FC ? shape_ : shape_.fft();

        const shared_t<real_t[]> buffer = input != output && all(indexing::isContiguous(output_stride, shape_fft)) ?
                                          std::reinterpret_pointer_cast<real_t[]>(output) :
                                          cuda::memory::PtrDevice<real_t>::alloc(shape_fft.elements(), stream);

        for (size_t batch = 0; batch < shape[0]; ++batch) {
            if constexpr (REMAP == Remap::F2F || REMAP == Remap::FC2FC) {
                standardizeFull_<REMAP>(input, input_stride, output, output_stride,
                                        buffer, shape_.stride(), shape_, norm, stream);
            } else if constexpr (REMAP == Remap::H2H || REMAP == Remap::HC2HC) {
                standardizeHalf_<REMAP>(input, input_stride, output, output_stride,
                                        buffer, shape_, shape_fft, norm, stream);
            } else {
                static_assert(noa::traits::always_false_v<T>);
            }
        }
    }

    #define INSTANTIATE_STANDARDIZE_(T)                                                                                                 \
    template void standardize<Remap::F2F, T>(const shared_t<T[]>&, size4_t, const shared_t<T[]>&, size4_t, size4_t, Norm, Stream&);     \
    template void standardize<Remap::FC2FC, T>(const shared_t<T[]>&, size4_t, const shared_t<T[]>&, size4_t, size4_t, Norm, Stream&);   \
    template void standardize<Remap::H2H, T>(const shared_t<T[]>&, size4_t, const shared_t<T[]>&, size4_t, size4_t, Norm, Stream&);     \
    template void standardize<Remap::HC2HC, T>(const shared_t<T[]>&, size4_t, const shared_t<T[]>&, size4_t, size4_t, Norm, Stream&)

    INSTANTIATE_STANDARDIZE_(cfloat_t);
    INSTANTIATE_STANDARDIZE_(cdouble_t);
}
