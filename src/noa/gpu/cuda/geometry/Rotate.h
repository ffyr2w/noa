#pragma once

#include <memory>

#include "noa/common/Definitions.h"
#include "noa/common/geometry/Euler.h"
#include "noa/common/geometry/Transform.h"
#include "noa/gpu/cuda/Types.h"
#include "noa/gpu/cuda/Stream.h"
#include "noa/gpu/cuda/memory/PtrPinned.h"
#include "noa/gpu/cuda/geometry/Transform.h"

namespace noa::cuda::geometry::details {
    template<int NDIM, typename T, typename R, typename C>
    constexpr bool is_valid_rotate_v =
            traits::is_any_v<T, float, cfloat_t> &&
            ((NDIM == 2 && traits::is_any_v<R, float, shared_t<float[]>> && traits::is_any_v<C, float2_t, shared_t<float2_t[]>>) ||
             (NDIM == 3 && traits::is_any_v<R, float33_t, shared_t<float33_t[]>> && traits::is_any_v<C, float3_t, shared_t<float3_t[]>>));
}

namespace noa::cuda::geometry {
    // Applies one or multiple 2D rotations.
    template<typename T, typename R, typename C, typename = std::enable_if_t<details::is_valid_rotate_v<2, T, R, C>>>
    void rotate2D(const shared_t<T[]>& input, dim4_t input_strides, dim4_t input_shape,
                  const shared_t<T[]>& output, dim4_t output_strides, dim4_t output_shape,
                  const R& rotations, const C& rotation_centers,
                  InterpMode interp_mode, BorderMode border_mode, bool prefilter, Stream& stream) {

        constexpr bool SINGLE_ROTATION = traits::is_float_v<R>;
        constexpr bool SINGLE_CENTER = traits::is_float2_v<C>;

        auto getInvertTransform_ = [=](dim_t index) {
            float2_t rotation_center;
            if constexpr (SINGLE_CENTER)
                rotation_center = rotation_centers;
            else
                rotation_center = rotation_centers.get()[index];

            float rotation;
            if constexpr (SINGLE_ROTATION)
                rotation = rotations;
            else
                rotation = rotations.get()[index];

            return float23_t(noa::geometry::translate(rotation_center) *
                             float33_t(noa::geometry::rotate(-rotation)) *
                             noa::geometry::translate(-rotation_center));
        };

        if ((SINGLE_CENTER && SINGLE_ROTATION) || output_shape[0] == 1) {
            transform2D(input, input_strides, input_shape, output, output_strides, output_shape,
                        getInvertTransform_(0), interp_mode, border_mode, prefilter, stream);
        } else {
            memory::PtrPinned<float23_t> inv_matrices(output_shape[0]);
            for (dim_t i = 0; i < output_shape[0]; ++i)
                inv_matrices[i] = getInvertTransform_(i);
            transform2D(input, input_strides, input_shape, output, output_strides, output_shape,
                        inv_matrices.share(), interp_mode, border_mode, prefilter, stream);
        }
    }

    // Applies one or multiple 3D rotations.
    template<typename T, typename R, typename C, typename = std::enable_if_t<details::is_valid_rotate_v<3, T, R, C>>>
    void rotate3D(const shared_t<T[]>& input, dim4_t input_strides, dim4_t input_shape,
                  const shared_t<T[]>& output, dim4_t output_strides, dim4_t output_shape,
                  const R& rotations, const C& rotation_centers,
                  InterpMode interp_mode, BorderMode border_mode, bool prefilter, Stream& stream) {

        constexpr bool SINGLE_ROTATION = traits::is_float33_v<R>;
        constexpr bool SINGLE_CENTER = traits::is_float3_v<C>;

        auto getInvertTransform_ = [=](dim_t index) {
            float3_t rotation_center;
            if constexpr (SINGLE_CENTER)
                rotation_center = rotation_centers;
            else
                rotation_center = rotation_centers.get()[index];

            float33_t rotation;
            if constexpr (SINGLE_ROTATION)
                rotation = rotations;
            else
                rotation = rotations.get()[index];

            return float34_t(noa::geometry::translate(rotation_center) *
                             float44_t(rotation) *
                             noa::geometry::translate(-rotation_center));
        };

        if ((SINGLE_CENTER && SINGLE_ROTATION) || output_shape[0] == 1) {
            transform3D(input, input_strides, input_shape, output, output_strides, output_shape,
                        getInvertTransform_(0), interp_mode, border_mode, prefilter, stream);
        } else {
            memory::PtrPinned<float34_t> inv_matrices(output_shape[0]);
            for (dim_t i = 0; i < output_shape[0]; ++i)
                inv_matrices[i] = getInvertTransform_(i);
            transform3D(input, input_strides, input_shape, output, output_strides, output_shape,
                        inv_matrices.share(), interp_mode, border_mode, prefilter, stream);
        }
    }
}
