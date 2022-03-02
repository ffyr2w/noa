#include "noa/common/Assert.h"
#include "noa/common/Types.h"
#include "noa/common/Exception.h"

#include "noa/cpu/memory/PtrHost.h"
#include "noa/cpu/transform/Interpolator.h"
#include "noa/cpu/transform/Prefilter.h"
#include "noa/cpu/transform/Apply.h"

namespace {
    using namespace ::noa;

    template<InterpMode INTERP, typename T>
    void transformWithSymmetry2D_(const T* input, size3_t input_stride, size3_t input_shape,
                                  T* output, size3_t output_stride, size3_t output_shape,
                                  float2_t shift, float22_t matrix, const geometry::Symmetry& symmetry, float2_t center,
                                  bool normalize, size_t threads) {
        const size_t count = symmetry.count();
        cpu::memory::PtrHost<float22_t> buffer(count);
        const float22_t* matrices_combined = buffer.data();
        const float33_t* matrices = symmetry.matrices();
        for (size_t i = 0; i < buffer.size(); ++i) {
            const float33_t& m = matrices[i];
            buffer[i] = float22_t{m[1][1], m[1][2],
                                  m[2][1], m[2][2]} * matrix;
        }

        const size_t offset = input_shape[0] == 1 ? 0 : input_stride[0];
        const size2_t stride{input_stride[1], input_stride[2]};
        const cpu::geometry::Interpolator2D<T> interp(input, stride, {input_shape[1], input_shape[2]}, 0);
        const float2_t center_shift = float2_t{center + shift};
        const auto scaling = normalize ? 1 / static_cast<traits::value_type_t<T>>(count + 1) : 1;

        #pragma omp parallel for collapse(3) default(none) num_threads(threads)     \
        shared(output, output_stride, output_shape, center, matrix, offset, interp, \
               center_shift, count, matrices_combined, scaling)

        for (size_t i = 0; i < output_shape[0]; ++i) {
            for (size_t y = 0; y < output_shape[1]; ++y) {
                for (size_t x = 0; x < output_shape[2]; ++x) {
                    float2_t pos{y, x};

                    pos -= center;
                    float2_t coordinates = matrix * pos;
                    coordinates += center_shift; // inverse shifts
                    T value = interp.template get<INTERP, BORDER_ZERO>(coordinates, i * offset);

                    for (size_t s = 0; s < count; ++s) {
                        coordinates = matrices_combined[s] * pos;
                        coordinates += center_shift;
                        value += interp.template get<INTERP, BORDER_ZERO>(coordinates, i * offset);
                    }

                    output[at(i, y, x, output_stride)] = value * scaling;
                }
            }
        }
    }

    template<InterpMode INTERP, typename T>
    void transformWithSymmetry3D_(const T* input, size4_t input_stride, size4_t input_shape,
                                  T* output, size4_t output_stride, size4_t output_shape,
                                  float3_t shift, float33_t matrix, const geometry::Symmetry& symmetry, float3_t center,
                                  bool normalize, size_t threads) {
        const size_t count = symmetry.count();
        cpu::memory::PtrHost<float33_t> buffer(count);
        const float33_t* matrices_combined = buffer.data();
        const float33_t* matrices = symmetry.matrices();
        for (size_t i = 0; i < buffer.size(); ++i)
            buffer[i] = matrices[i] * matrix;

        const size_t offset = input_shape[0] == 1 ? 0 : input_stride[0];
        const size3_t stride{input_stride.get() + 1};
        const cpu::geometry::Interpolator3D<T> interp(input, stride, size3_t{input_shape.get() + 1}, 0);
        const float3_t center_shift = float3_t{center + shift};
        const auto scaling = normalize ? 1 / static_cast<traits::value_type_t<T>>(count + 1) : 1;

        #pragma omp parallel for collapse(4) default(none) num_threads(threads)     \
        shared(output, output_stride, output_shape, center, matrix, offset, interp, \
               center_shift, count, matrices_combined, scaling)

        for (size_t i = 0; i < output_shape[0]; ++i) {
            for (size_t z = 0; z < output_shape[1]; ++z) {
                for (size_t y = 0; y < output_shape[2]; ++y) {
                    for (size_t x = 0; x < output_shape[3]; ++x) {
                        float3_t pos(z, y, x);

                        pos -= center;
                        float3_t coordinates = matrix * pos;
                        coordinates += center_shift;
                        T value = interp.template get<INTERP, BORDER_ZERO>(coordinates, i * offset);

                        for (size_t s = 0; s < count; ++s) {
                            coordinates = matrices_combined[s] * pos;
                            coordinates += center_shift;
                            value += interp.template get<INTERP, BORDER_ZERO>(coordinates, i * offset);
                        }

                        output[at(i, z, y, x, output_stride)] = value * scaling;
                    }
                }
            }
        }
    }
}

namespace noa::cpu::geometry {
    template<bool PREFILTER, typename T>
    void transform2D(const T* input, size4_t input_stride, size4_t input_shape,
                     T* output, size4_t output_stride, size4_t output_shape,
                     float2_t shift, float22_t matrix, const Symmetry& symmetry, float2_t center,
                     InterpMode interp_mode, bool normalize, Stream& stream) {
        NOA_ASSERT(input != output);
        NOA_ASSERT(input_shape[1] == 1 && output_shape[1] == 1);
        NOA_ASSERT(input_shape[0] == 1 || input_shape[0] == output_shape[0]);

        const size_t threads = stream.threads();
        stream.enqueue([=, &symmetry, &stream]() {
            memory::PtrHost<T> buffer;
            const T* tmp;
            size3_t istride; // assume Z == 1
            if (PREFILTER && interp_mode == INTERP_CUBIC_BSPLINE) {
                // FIXME There's no point to support input broadcast since there's only one transform.
                size4_t shape = input_shape;
                if (input_stride[0] == 0)
                    shape[0] = 1;
                const size4_t stride = shape.strides();
                buffer.reset(shape.elements());
                bspline::prefilter(input, input_stride, buffer.get(), stride, shape, stream);
                tmp = buffer.get();
                istride = {stride[0], stride[2], stride[3]};
            } else {
                tmp = input;
                istride = {input_stride[0], input_stride[2], input_stride[3]};
            }

            const size3_t ishape{input_shape[0], input_shape[2], input_shape[3]};
            const size3_t oshape{output_shape[0], output_shape[2], output_shape[3]};
            const size3_t ostride{output_stride[0], output_stride[2], output_stride[3]};
            switch (interp_mode) {
                case INTERP_NEAREST:
                    return transformWithSymmetry2D_<INTERP_NEAREST, T>(
                            tmp, istride, ishape, output, ostride, oshape,
                            shift, matrix, symmetry, center, normalize, threads);
                case INTERP_LINEAR:
                    return transformWithSymmetry2D_<INTERP_LINEAR, T>(
                            tmp, istride, ishape, output, ostride, oshape,
                            shift, matrix, symmetry, center, normalize, threads);
                case INTERP_COSINE:
                    return transformWithSymmetry2D_<INTERP_COSINE, T>(
                            tmp, istride, ishape, output, ostride, oshape,
                            shift, matrix, symmetry, center, normalize, threads);
                case INTERP_CUBIC:
                    return transformWithSymmetry2D_<INTERP_CUBIC, T>(
                            tmp, istride, ishape, output, ostride, oshape,
                            shift, matrix, symmetry, center, normalize, threads);
                case INTERP_CUBIC_BSPLINE:
                    return transformWithSymmetry2D_<INTERP_CUBIC_BSPLINE, T>(
                            tmp, istride, ishape, output, ostride, oshape,
                            shift, matrix, symmetry, center, normalize, threads);
                default:
                    NOA_THROW("The interpolation/filter mode {} is not supported", interp_mode);
            }
        });
    }

    template<bool PREFILTER, typename T>
    void transform3D(const T* input, size4_t input_stride, size4_t input_shape,
                     T* output, size4_t output_stride, size4_t output_shape,
                     float3_t shift, float33_t matrix, const Symmetry& symmetry, float3_t center,
                     InterpMode interp_mode, bool normalize, Stream& stream) {
        NOA_ASSERT(input != output);
        NOA_ASSERT(input_shape[0] == 1 || input_shape[0] == output_shape[0]);

        const size_t threads = stream.threads();
        stream.enqueue([=, &symmetry, &stream]() {
            memory::PtrHost<T> buffer;
            const T* tmp;
            size4_t tmp_stride;
            if (PREFILTER && interp_mode == INTERP_CUBIC_BSPLINE) {
                // FIXME There's no point to support input broadcast since there's only one transform.
                size4_t shape = input_shape;
                if (input_stride[0] == 0)
                    shape[0] = 1;
                const size4_t stride = shape.strides();
                buffer.reset(shape.elements());
                bspline::prefilter(input, input_stride, buffer.get(), stride, shape, stream);
                tmp = buffer.get();
                tmp_stride = stride;
            } else {
                tmp = input;
                tmp_stride = input_stride;
            }
            switch (interp_mode) {
                case INTERP_NEAREST:
                    return transformWithSymmetry3D_<INTERP_NEAREST, T>(
                            tmp, tmp_stride, input_shape, output, output_stride, output_shape,
                            shift, matrix, symmetry, center, normalize, threads);
                case INTERP_LINEAR:
                    return transformWithSymmetry3D_<INTERP_LINEAR, T>(
                            tmp, tmp_stride, input_shape, output, output_stride, output_shape,
                            shift, matrix, symmetry, center, normalize, threads);
                case INTERP_COSINE:
                    return transformWithSymmetry3D_<INTERP_COSINE, T>(
                            tmp, tmp_stride, input_shape, output, output_stride, output_shape,
                            shift, matrix, symmetry, center, normalize, threads);
                case INTERP_CUBIC:
                    return transformWithSymmetry3D_<INTERP_CUBIC, T>(
                            tmp, tmp_stride, input_shape, output, output_stride, output_shape,
                            shift, matrix, symmetry, center, normalize, threads);
                case INTERP_CUBIC_BSPLINE:
                    return transformWithSymmetry3D_<INTERP_CUBIC_BSPLINE, T>(
                            tmp, tmp_stride, input_shape, output, output_stride, output_shape,
                            shift, matrix, symmetry, center, normalize, threads);
                default:
                    NOA_THROW("The interpolation/filter mode {} is not supported", interp_mode);
            }
        });
    }

    #define NOA_INSTANTIATE_APPLY_(T)                                                                                                                                   \
    template void transform2D<true, T>(const T*, size4_t, size4_t, T*, size4_t, size4_t, float2_t, float22_t, const Symmetry&, float2_t, InterpMode, bool, Stream&);    \
    template void transform3D<true, T>(const T*, size4_t, size4_t, T*, size4_t, size4_t, float3_t, float33_t, const Symmetry&, float3_t, InterpMode, bool, Stream&);    \
    template void transform2D<false, T>(const T*, size4_t, size4_t, T*, size4_t, size4_t, float2_t, float22_t, const Symmetry&, float2_t, InterpMode, bool, Stream&);   \
    template void transform3D<false, T>(const T*, size4_t, size4_t, T*, size4_t, size4_t, float3_t, float33_t, const Symmetry&, float3_t, InterpMode, bool, Stream&)

    NOA_INSTANTIATE_APPLY_(float);
    NOA_INSTANTIATE_APPLY_(double);
    NOA_INSTANTIATE_APPLY_(cfloat_t);
    NOA_INSTANTIATE_APPLY_(cdouble_t);
}
