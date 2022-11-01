#include "noa/common/Assert.h"
#include "noa/gpu/cuda/Exception.h"

#include "noa/gpu/cuda/memory/Copy.h"
#include "noa/gpu/cuda/memory/PtrArray.h"
#include "noa/gpu/cuda/memory/PtrDevice.h"
#include "noa/gpu/cuda/memory/PtrTexture.h"

#include "noa/gpu/cuda/geometry/Transform.h"
#include "noa/gpu/cuda/geometry/Interpolator.h"
#include "noa/gpu/cuda/geometry/Prefilter.h"

namespace {
    using namespace ::noa;
    constexpr dim3 THREADS(16, 16);

    template<typename T, typename interpolator_t>
    __global__ void __launch_bounds__(THREADS.x * THREADS.y)
    transformWithSymmetry2D_(interpolator_t interpolator, Accessor<T, 3, uint32_t> output, uint2_t shape,
                             float2_t shift, float22_t matrix, float2_t center,
                             const float33_t* symmetry_matrices, int32_t symmetry_count, float scaling) {
        const uint3_t gid{blockIdx.z,
                          blockIdx.y * THREADS.y + threadIdx.y,
                          blockIdx.x * THREADS.x + threadIdx.x};
        if (gid[1] >= shape[0] || gid[2] >= shape[1])
            return;

        float2_t coordinates{gid[1], gid[2]};
        coordinates -= center;
        coordinates = matrix * coordinates;
        T value = interpolator(coordinates + center + shift);
        for (int32_t i = 0; i < symmetry_count; ++i) {
            const float33_t& m = symmetry_matrices[i];
            const float22_t sym_matrix{m[1][1], m[1][2],
                                       m[2][1], m[2][2]};
            const float2_t i_coordinates = sym_matrix * coordinates;
            value += interpolator(i_coordinates + center + shift);
        }

        output(gid) = value * scaling;
    }

    template<typename T>
    void launchTransformSymmetryTexture2D_(cudaTextureObject_t texture, InterpMode texture_interp_mode,
                                           T* output, dim4_t output_strides, dim4_t output_shape,
                                           float2_t shift, float22_t matrix, const geometry::Symmetry& symmetry,
                                           float2_t center, bool normalize, cuda::Stream& stream) {
        // TODO Move symmetry matrices to constant memory?
        const dim_t count = symmetry.count();
        const float33_t* symmetry_matrices = symmetry.get();
        using unique_ptr = cuda::memory::PtrDevice<float33_t>::alloc_unique_t;
        unique_ptr d_matrices = cuda::memory::PtrDevice<float33_t>::alloc(count, stream);
        cuda::memory::copy(symmetry_matrices, d_matrices.get(), count, stream);
        const float scaling = normalize ? 1 / static_cast<float>(count + 1) : 1;

        const auto o_shape = safe_cast<uint2_t>(dim2_t(output_shape.get(2)));
        const auto o_strides = safe_cast<uint3_t>(dim3_t{output_strides[0], output_strides[2], output_strides[3]});
        const dim3 blocks(math::divideUp(o_shape[1], THREADS.x),
                          math::divideUp(o_shape[0], THREADS.y),
                          output_shape[0]);
        const cuda::LaunchConfig config{blocks, THREADS};
        const Accessor<T, 3, uint32_t> output_accessor(output, o_strides);

        switch (texture_interp_mode) {
            case INTERP_NEAREST: {
                using interpolator_t = cuda::geometry::Interpolator2D<INTERP_NEAREST, T>;
                return stream.enqueue("geometry::transform2D", transformWithSymmetry2D_<T, interpolator_t>, config,
                                      interpolator_t(texture), output_accessor, o_shape, shift, matrix, center,
                                      d_matrices.get(), count, scaling);
            }
            case INTERP_LINEAR: {
                using interpolator_t = cuda::geometry::Interpolator2D<INTERP_LINEAR, T>;
                return stream.enqueue("geometry::transform2D", transformWithSymmetry2D_<T, interpolator_t>, config,
                                      interpolator_t(texture), output_accessor, o_shape, shift, matrix, center,
                                      d_matrices.get(), count, scaling);
            }
            case INTERP_COSINE: {
                using interpolator_t = cuda::geometry::Interpolator2D<INTERP_COSINE, T>;
                return stream.enqueue("geometry::transform2D", transformWithSymmetry2D_<T, interpolator_t>, config,
                                      interpolator_t(texture), output_accessor, o_shape, shift, matrix, center,
                                      d_matrices.get(), count, scaling);
            }
            case INTERP_CUBIC: {
                using interpolator_t = cuda::geometry::Interpolator2D<INTERP_CUBIC, T>;
                return stream.enqueue("geometry::transform2D", transformWithSymmetry2D_<T, interpolator_t>, config,
                                      interpolator_t(texture), output_accessor, o_shape, shift, matrix, center,
                                      d_matrices.get(), count, scaling);
            }
            case INTERP_CUBIC_BSPLINE: {
                using interpolator_t = cuda::geometry::Interpolator2D<INTERP_CUBIC_BSPLINE, T>;
                return stream.enqueue("geometry::transform2D", transformWithSymmetry2D_<T, interpolator_t>, config,
                                      interpolator_t(texture), output_accessor, o_shape, shift, matrix, center,
                                      d_matrices.get(), count, scaling);
            }
            case INTERP_LINEAR_FAST: {
                using interpolator_t = cuda::geometry::Interpolator2D<INTERP_LINEAR_FAST, T>;
                return stream.enqueue("geometry::transform2D", transformWithSymmetry2D_<T, interpolator_t>, config,
                                      interpolator_t(texture), output_accessor, o_shape, shift, matrix, center,
                                      d_matrices.get(), count, scaling);
            }
            case INTERP_COSINE_FAST: {
                using interpolator_t = cuda::geometry::Interpolator2D<INTERP_COSINE_FAST, T>;
                return stream.enqueue("geometry::transform2D", transformWithSymmetry2D_<T, interpolator_t>, config,
                                      interpolator_t(texture), output_accessor, o_shape, shift, matrix, center,
                                      d_matrices.get(), count, scaling);
            }
            case INTERP_CUBIC_BSPLINE_FAST: {
                using interpolator_t = cuda::geometry::Interpolator2D<INTERP_CUBIC_BSPLINE_FAST, T>;
                return stream.enqueue("geometry::transform2D", transformWithSymmetry2D_<T, interpolator_t>, config,
                                      interpolator_t(texture), output_accessor, o_shape, shift, matrix, center,
                                      d_matrices.get(), count, scaling);
            }
        }
    }
}

namespace noa::cuda::geometry {
    template<typename T, typename>
    void transform2D(const shared_t<T[]>& input, dim4_t input_strides, dim4_t input_shape,
                     const shared_t<T[]>& output, dim4_t output_strides, dim4_t output_shape,
                     float2_t shift, float22_t matrix, const Symmetry& symmetry, float2_t center,
                     InterpMode interp_mode, bool prefilter, bool normalize, Stream& stream) {
        NOA_ASSERT(input && all(input_shape > 0) && all(output_shape > 0));
        NOA_ASSERT_DEVICE_PTR(output.get(), stream.device());
        NOA_ASSERT(input_shape[0] == 1 || input_shape[0] == output_shape[0]);
        NOA_ASSERT(input_shape[1] == 1 && output_shape[1] == 1);

        if (input_strides[0] == 0)
            input_shape[0] = 1;
        else if (input_shape[0] == 1)
            input_strides[0] = 0;

        // Prepare the input array:
        shared_t<T[]> buffer;
        const T* buffer_ptr;
        dim_t buffer_pitch;
        dim_t buffer_offset;
        if (prefilter && (interp_mode == INTERP_CUBIC_BSPLINE || interp_mode == INTERP_CUBIC_BSPLINE_FAST)) {
            if (input_shape[2] != output_shape[2] || input_shape[3] != output_shape[3]) {
                buffer = memory::PtrDevice<T>::alloc(input_shape.elements(), stream);
                const dim4_t contiguous_strides = input_shape.strides();
                bspline::prefilter(input, input_strides,
                                                   buffer, contiguous_strides, input_shape, stream);
                buffer_ptr = buffer.get();
                buffer_pitch = contiguous_strides[2];
                buffer_offset = contiguous_strides[0];
            } else {
                NOA_ASSERT(indexing::isContiguous(output_strides, output_shape)[3]);
                // Whether input is batched or not, since we copy to the CUDA array, we can use the output as buffer.
                bspline::prefilter(input, input_strides, output, output_strides, input_shape, stream);
                buffer_ptr = output.get();
                buffer_pitch = output_strides[2];
                buffer_offset = output_strides[0];
            }
        } else {
            NOA_ASSERT(indexing::isContiguous(input_strides, input_shape)[3]);
            buffer_ptr = input.get();
            buffer_pitch = input_strides[2];
            buffer_offset = input_strides[0];
        }

        // Broadcast input if it is not batched:
        const dim4_t o_shape{input_shape[0] > 1 ? 1 : output_shape[0],
                             output_shape[1], output_shape[2], output_shape[3]};

        // Copy to texture and launch (per input batch):
        const dim3_t shape_3d{1, input_shape[2], input_shape[3]};
        cuda::memory::PtrArray<T> array(shape_3d);
        cuda::memory::PtrTexture texture(array.get(), interp_mode, BORDER_ZERO);
        for (dim_t i = 0; i < input_shape[0]; ++i) {
            cuda::memory::copy(buffer_ptr + i * buffer_offset, buffer_pitch, array.get(), shape_3d, stream);
            launchTransformSymmetryTexture2D_(
                    texture.get(), interp_mode, output.get() + i * output_strides[0], output_strides, o_shape,
                    shift, matrix, symmetry, center, normalize, stream);
        }
        stream.attach(input, output, symmetry.share(), array.share(), texture.share());
        if (buffer)
            stream.attach(buffer);
    }

    template<typename T, typename>
    void transform2D(const shared_t<cudaArray>& array,
                     const shared_t<cudaTextureObject_t>& texture, InterpMode texture_interp_mode,
                     const shared_t<T[]>& output, dim4_t output_strides, dim4_t output_shape,
                     float2_t shift, float22_t matrix, const Symmetry& symmetry, float2_t center,
                     bool normalize, Stream& stream) {
        NOA_ASSERT(array && texture && all(output_shape > 0));
        NOA_ASSERT_DEVICE_PTR(output.get(), stream.device());
        launchTransformSymmetryTexture2D_(*texture, texture_interp_mode,
                                          output.get(), output_strides, output_shape,
                                          shift, matrix, symmetry, center, normalize, stream);
        stream.attach(array, texture, output, symmetry.share());
    }

    #define NOA_INSTANTIATE_TRANSFORM_SYM_(T)                                                                                                                                                           \
    template void transform2D<T, void>(const shared_t<T[]>&, dim4_t, dim4_t, const shared_t<T[]>&, dim4_t, dim4_t, float2_t, float22_t, const Symmetry&, float2_t, InterpMode, bool, bool, Stream&);    \
    template void transform2D<T, void>(const shared_t<cudaArray>&, const shared_t<cudaTextureObject_t>&, InterpMode, const shared_t<T[]>&, dim4_t, dim4_t, float2_t, float22_t, const Symmetry&, float2_t, bool, Stream&)

    NOA_INSTANTIATE_TRANSFORM_SYM_(float);
    NOA_INSTANTIATE_TRANSFORM_SYM_(cfloat_t);
}
