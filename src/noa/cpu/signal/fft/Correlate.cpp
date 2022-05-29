#include "noa/cpu/fft/Transforms.h"
#include "noa/cpu/math/Ewise.h"
#include "noa/cpu/math/Find.h"
#include "noa/cpu/math/Reduce.h"
#include "noa/cpu/memory/PtrHost.h"
#include "noa/cpu/signal/fft/Correlate.h"
#include "noa/cpu/signal/fft/Shift.h"

namespace {
    using namespace ::noa;

    // From the DC-centered frequency to a valid index in the non-centered output.
    // The input frequency should be in-bound, i.e. -n/2 <= frequency <= (n-1)/2
    inline constexpr int64_t getIndex_(int64_t frequency, int64_t volume_dim) noexcept {
        return frequency < 0 ? volume_dim + frequency : frequency;
    }

    // From a valid index to the DC-centered frequency.
    inline constexpr long3_t getFrequency_(long3_t index, long3_t shape) noexcept {
        return {index[0] < (shape[0] + 1) / 2 ? index[0] : index[0] - shape[0],
                index[1] < (shape[1] + 1) / 2 ? index[1] : index[1] - shape[1],
                index[2] < (shape[2] + 1) / 2 ? index[2] : index[2] - shape[2]};
    }

    // From a valid index to the DC-centered frequency.
    inline constexpr long2_t getFrequency_(long2_t index, long2_t shape) noexcept {
        return {index[0] < (shape[0] + 1) / 2 ? index[0] : index[0] - shape[0],
                index[1] < (shape[1] + 1) / 2 ? index[1] : index[1] - shape[1]};
    }

    // Given values at three successive positions, y[0], y[1], y[2], where
    // y[1] is the peak value, this fits a parabola to the values and returns the
    // offset (from -0.5 to 0.5) from the center position.
    template<typename T>
    inline constexpr T getParabolicVertex_(T* y) noexcept {
        // From IMOD/libcfshr/filtxcorr.c::parabolicFitPosition
        const T d = 2 * (y[0] + y[2] - 2 * y[1]);
        T x = 0;
        if (math::abs(d) > math::abs(static_cast<T>(1e-2) * (y[0] - y[2])))
            x = (y[0] - y[2]) / d;
        if (x > T{0.5})
            x = T{0.5};
        if (x < T{-0.5})
            x = T{-0.5};
        return x;
    }

    template<int NDIM, fft::Remap REMAP, typename T, typename U>
    inline constexpr auto getSinglePeak(const T* input, U stride, U shape, U peak) {
        static_assert((NDIM == 2 && traits::is_almost_same_v<U, long2_t>) ||
                      (NDIM == 3 && traits::is_almost_same_v<U, long3_t>));
        constexpr size_t ELEMENTS = 3 * NDIM;

        std::array<T, ELEMENTS> output{0};
        T* output_ptr = output.data();

        if constexpr (REMAP == fft::F2F) {
            const U peak_ = getFrequency_(peak, shape);
            for (int dim = 0; dim < NDIM; ++dim) {
                const T* input_ = input;

                // Add peak offset in the other dimensions.
                for (int i = 0; i < NDIM; ++i)
                    input_ += peak[i] * stride[i] * (dim != i);

                // Here the 2 adjacent points might be in a separate quadrant.
                // Retrieve the frequency, add the offset, and if it's a valid
                // frequency, convert back to an index.
                for (int l = -(NDIM - 1); l < NDIM; ++l, ++output_ptr) {
                    const int64_t il = peak_[dim] + l;
                    if (-shape[dim] / 2 <= il && il <= (shape[dim] - 1) / 2)
                        *output_ptr = input_[getIndex_(il, shape[dim]) * stride[dim]];
                }
            }
            // The CC map was not centered, so center it here.
            for (int dim = 0; dim < NDIM; ++dim)
                peak[dim] = math::FFTShift(peak[dim], shape[dim]);

        } else {
            input += indexing::at(peak, stride);
            for (int dim = 0; dim < NDIM; ++dim) {
                for (int l = -(NDIM - 1); l < NDIM; ++l, ++output_ptr) {
                    const int64_t il = peak[dim] + l;
                    if (il >= 0 && il < shape[dim])
                        *output_ptr = input[l * stride[dim]];
                }
            }
        }

        // Add sub-pixel position by fitting a 1D parabola to the peak and its 2 adjacent points.
        using peak_type = std::conditional_t<NDIM == 2, float2_t, float3_t>;
        peak_type final_peak{peak};
        for (int dim = 0; dim < NDIM; ++dim)
            final_peak[dim] += static_cast<float>(getParabolicVertex_(output.data() + 3 * dim));
        return final_peak;
    }
}

namespace noa::cpu::signal::fft {
    template<Remap REMAP, typename T, typename>
    void xmap(const shared_t<Complex<T>[]>& lhs, size4_t lhs_stride,
              const shared_t<Complex<T>[]>& rhs, size4_t rhs_stride,
              const shared_t<T[]>& output, size4_t output_stride,
              size4_t shape, bool normalize, Norm norm, Stream& stream,
              const shared_t<Complex<T>[]>& tmp, size4_t tmp_stride) {

        const shared_t<Complex<T>[]>& buffer = tmp ? tmp : rhs;
        const size4_t& buffer_stride = tmp ? tmp_stride : rhs_stride;
        NOA_ASSERT(all(buffer_stride > 0));

        if (normalize) {
            cpu::math::ewise(
                    lhs, lhs_stride, rhs, rhs_stride, buffer, buffer_stride, shape.fft(),
                    [](Complex<T> l, Complex<T> r) {
                        const Complex<T> product = l * noa::math::conj(r);
                        const T magnitude = noa::math::abs(product);
                        return magnitude < static_cast<T>(1e-7) ? 0 : product / magnitude;
                    }, stream);
        } else {
            cpu::math::ewise(lhs, lhs_stride, rhs, rhs_stride, buffer, buffer_stride,
                             shape.fft(), noa::math::multiply_conj_t{}, stream);
        }

        if constexpr (REMAP == Remap::H2FC) {
            const size3_t shape_3d{shape.get() + 1};
            if (shape_3d.ndim() == 3) {
                cpu::signal::fft::shift3D<Remap::H2H>(buffer, buffer_stride, buffer, buffer_stride, shape,
                                                      float3_t{shape_3d / 2}, 1, stream);
            } else {
                cpu::signal::fft::shift2D<Remap::H2H>(buffer, buffer_stride, buffer, buffer_stride, shape,
                                                      float2_t{shape_3d[1] / 2, shape_3d[2] / 2}, 1, stream);
            }
        }

        cpu::fft::c2r(buffer, buffer_stride, output, output_stride, shape, cpu::fft::ESTIMATE, norm, stream);
    }

    template<Remap REMAP, typename T, typename>
    void xpeak2D(const shared_t<T[]>& xmap, size4_t stride, size4_t shape,
                 const shared_t<float2_t[]>& peaks, Stream& stream) {
        stream.enqueue([=]() mutable {
            cpu::memory::PtrHost<int64_t> offsets{shape[0]};
            math::find(noa::math::max_t{}, xmap, stride, shape, offsets.share(), true, stream);
            const auto pitch = static_cast<int64_t>(stride[2]);
            const long2_t shape_2d{shape.get() + 2};
            const long2_t stride_2d{stride.get() + 2};
            for (size_t batch = 0; batch < shape[0]; ++batch) {
                const long2_t peak = noa::indexing::indexes(offsets[batch], pitch);
                const T* imap = xmap.get() + stride[0] * batch;
                peaks.get()[batch] = getSinglePeak<2, REMAP>(imap, stride_2d, shape_2d, peak);
            }
        });
    }

    template<Remap REMAP, typename T, typename>
    float2_t xpeak2D(const shared_t<T[]>& xmap, size4_t stride, size4_t shape, Stream& stream) {
        NOA_ASSERT(shape.ndim() == 2);
        const auto offset = math::find<int64_t>(noa::math::max_t{}, xmap, stride, shape, stream);
        const auto pitch = static_cast<int64_t>(stride[2]);
        const long2_t shape_2d{shape.get() + 2};
        const long2_t stride_2d{stride.get() + 2};
        const long2_t peak = noa::indexing::indexes(offset, pitch);
        return getSinglePeak<2, REMAP>(xmap.get(), stride_2d, shape_2d, peak);
    }

    template<Remap REMAP, typename T, typename>
    void xpeak3D(const shared_t<T[]>& xmap, size4_t stride, size4_t shape,
                 const shared_t<float3_t[]>& peaks, Stream& stream) {
        stream.enqueue([=]() mutable {
            cpu::memory::PtrHost<int64_t> offsets{shape[0]};
            math::find(noa::math::max_t{}, xmap, stride, shape, offsets.share(), true, stream);
            const long2_t pitch{stride[1] / stride[2], stride[2]};
            const long3_t shape_3d{shape.get() + 1};
            const long3_t stride_3d{stride.get() + 1};
            for (size_t batch = 0; batch < shape[0]; ++batch) {
                const long3_t peak = noa::indexing::indexes(offsets[batch], pitch[0], pitch[1]);
                const T* imap = xmap.get() + stride[0] * batch;
                peaks.get()[batch] = getSinglePeak<3, REMAP>(imap, stride_3d, shape_3d, peak);
            }
        });
    }

    template<Remap REMAP, typename T, typename>
    float3_t xpeak3D(const shared_t<T[]>& xmap, size4_t stride, size4_t shape, Stream& stream) {
        const auto offset = math::find<int64_t>(noa::math::max_t{}, xmap, stride, shape, stream);
        const long2_t pitch{stride[1] / stride[2], stride[2]};
        const long3_t shape_3d{shape.get() + 1};
        const long3_t stride_3d{stride.get() + 1};
        const long3_t peak = noa::indexing::indexes(offset, pitch[0], pitch[1]);
        return getSinglePeak<3, REMAP>(xmap.get(), stride_3d, shape_3d, peak);
    }

    #define INSTANTIATE_XMAP(T)                                                                                                                                                \
    template void xmap<Remap::H2F, T, void>(const shared_t<Complex<T>[]>&, size4_t, const shared_t<Complex<T>[]>&, size4_t, const shared_t<T[]>&, size4_t, size4_t, bool, Norm, Stream&, const shared_t<Complex<T>[]>&, size4_t);   \
    template void xmap<Remap::H2FC, T, void>(const shared_t<Complex<T>[]>&, size4_t, const shared_t<Complex<T>[]>&, size4_t, const shared_t<T[]>&, size4_t, size4_t, bool, Norm, Stream&, const shared_t<Complex<T>[]>&, size4_t);  \
    template void xpeak2D<Remap::F2F, T, void>(const shared_t<T[]>&, size4_t, size4_t, const shared_t<float2_t[]>&, Stream&);     \
    template void xpeak2D<Remap::FC2FC, T, void>(const shared_t<T[]>&, size4_t, size4_t, const shared_t<float2_t[]>&, Stream&);   \
    template void xpeak3D<Remap::F2F, T, void>(const shared_t<T[]>&, size4_t, size4_t, const shared_t<float3_t[]>&, Stream&);     \
    template void xpeak3D<Remap::FC2FC, T, void>(const shared_t<T[]>&, size4_t, size4_t, const shared_t<float3_t[]>&, Stream&);   \
    template float2_t xpeak2D<Remap::F2F, T, void>(const shared_t<T[]>&, size4_t, size4_t, Stream&);     \
    template float2_t xpeak2D<Remap::FC2FC, T, void>(const shared_t<T[]>&, size4_t, size4_t, Stream&);   \
    template float3_t xpeak3D<Remap::F2F, T, void>(const shared_t<T[]>&, size4_t, size4_t, Stream&);     \
    template float3_t xpeak3D<Remap::FC2FC, T, void>(const shared_t<T[]>&, size4_t, size4_t, Stream&)

    INSTANTIATE_XMAP(float);
    INSTANTIATE_XMAP(double);
}

namespace noa::cpu::signal::fft::details {
    template<typename T>
    T xcorr(const Complex<T>* lhs, size3_t lhs_stride,
            const Complex<T>* rhs, size3_t rhs_stride,
            size3_t shape, size_t threads) {
        double sum = 0, sum_lhs = 0, sum_rhs = 0;
        double err = 0, err_lhs = 0, err_rhs = 0;

        auto abs_sqd = noa::math::abs_squared_t{};
        auto reduce_sum = [](double value, double& s, double& e) {
            auto t = s + value;
            e += noa::math::abs(s) >= noa::math::abs(value) ? (s - t) + value : (value - t) + s;
            s = t;
        };

        #pragma omp parallel for collapse(3) num_threads(threads)               \
        reduction(+:sum, sum_lhs, sum_rhs, err, err_lhs, err_rhs) default(none) \
        shared(lhs, lhs_stride, rhs, rhs_stride, shape, abs_sqd, reduce_sum)

        for (size_t j = 0; j < shape[0]; ++j) {
            for (size_t k = 0; k < shape[1]; ++k) {
                for (size_t l = 0; l < shape[2]; ++l) {

                    const auto lhs_value = static_cast<cdouble_t>(lhs[indexing::at(j, k, l, lhs_stride)]);
                    const auto rhs_value = static_cast<cdouble_t>(rhs[indexing::at(j, k, l, rhs_stride)]);

                    reduce_sum(abs_sqd(lhs_value), sum_lhs, err_lhs);
                    reduce_sum(abs_sqd(rhs_value), sum_rhs, err_rhs);
                    reduce_sum(noa::math::real(lhs_value * noa::math::conj(rhs_value)), sum, err);
                }
            }
        }

        const double numerator = sum + err;
        const double denominator = noa::math::sqrt((sum_lhs + err_lhs) * (sum_rhs + err_rhs));
        return static_cast<T>(numerator / denominator);
    }

    #define INSTANTIATE_XCORR(T) \
    template T xcorr<T>(const Complex<T>*, size3_t, const Complex<T>*, size3_t, size3_t, size_t)

    INSTANTIATE_XCORR(float);
    INSTANTIATE_XCORR(double);
}
