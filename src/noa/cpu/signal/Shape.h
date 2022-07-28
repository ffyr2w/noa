/// \file noa/cpu/mask/Shape.h
/// \brief Shape masks.
/// \author Thomas - ffyr2w
/// \date 18 Jun 2021

#pragma once

#include "noa/common/Definitions.h"
#include "noa/common/Types.h"
#include "noa/cpu/Stream.h"

namespace noa::cpu::signal {
    /// Returns or applies a spherical mask.
    /// \tparam INVERT          Whether the mask should be inverted. If true, everything within the sphere is removed.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        On the \b host. Array to mask. If nullptr, write mask in \p output.
    /// \param input_strides    BDHW strides, in elements, of \p input.
    /// \param[out] output      On the \b host. Masked array. Can be equal to \p input.
    /// \param output_strides   BDHW strides, in elements, of \p output.
    /// \param shape            BDHW shape of \p input and \p output.
    /// \param center           DHW center of the sphere.
    /// \param radius           Radius, in elements, of the sphere.
    /// \param taper_size       Width, in elements, of the raised-cosine, including the first zero.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    template<bool INVERT = false, typename T,
             typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void sphere(const shared_t<T[]>& input, size4_t input_strides,
                const shared_t<T[]>& output, size4_t output_strides, size4_t shape,
                float3_t center, float radius, float taper_size, Stream& stream);

    /// Returns or applies a 2D spherical mask.
    /// \tparam INVERT          Whether the mask should be inverted. If true, everything within the sphere is removed.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        On the \b host. Array to mask. If nullptr, write mask in \p output.
    /// \param input_strides    BDHW strides, in elements, of \p input.
    /// \param[out] output      On the \b host. Masked array. Can be equal to \p input.
    /// \param output_strides   BDHW strides, in elements, of \p output.
    /// \param shape            BDHW shape of \p input and \p output.
    /// \param center           HW center of the sphere.
    /// \param radius           Radius, in elements, of the sphere.
    /// \param taper_size       Width, in elements, of the raised-cosine, including the first zero.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    template<bool INVERT = false, typename T,
             typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void sphere(const shared_t<T[]>& input, size4_t input_strides,
                const shared_t<T[]>& output, size4_t output_strides, size4_t shape,
                float2_t center, float radius, float taper_size, Stream& stream) {
        return sphere<INVERT>(input, input_strides, output, output_strides, shape,
                              float3_t{0, center[0], center[1]}, radius,
                              taper_size, stream);
    }

    /// Returns or applies a rectangular mask.
    /// \tparam INVERT          Whether the mask should be inverted. If true, everything within the rectangle is removed.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        On the \b host. Array to mask. If nullptr, write mask in \p output.
    /// \param input_strides    BDHW strides, in elements, of \p input.
    /// \param[out] output      On the \b host. Masked array. Can be equal to \p input.
    /// \param output_strides   BDHW strides, in elements, of \p output.
    /// \param shape            BDHW shape of \p input and \p output.
    /// \param center           DHW center of the rectangle.
    /// \param radius           DHW radius, in elements, of the rectangle.
    /// \param taper_size       Width, in elements, of the raised-cosine, including the first zero.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    template<bool INVERT = false, typename T,
             typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void rectangle(const shared_t<T[]>& input, size4_t input_strides,
                   const shared_t<T[]>& output, size4_t output_strides, size4_t shape,
                   float3_t center, float3_t radius, float taper_size, Stream& stream);

    /// Returns or applies a 2D rectangular mask.
    /// \tparam INVERT          Whether the mask should be inverted. If true, everything within the rectangle is removed.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        On the \b host. Array to mask. If nullptr, write mask in \p output.
    /// \param input_strides    BDHW strides, in elements, of \p input.
    /// \param[out] output      On the \b host. Masked array. Can be equal to \p input.
    /// \param output_strides   BDHW strides, in elements, of \p output.
    /// \param shape            BDHW shape of \p input and \p output.
    /// \param center           HW center of the rectangle.
    /// \param radius           HW radius, in elements, of the rectangle.
    /// \param taper_size       Width, in elements, of the raised-cosine, including the first zero.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    template<bool INVERT = false, typename T,
             typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void rectangle(const shared_t<T[]>& input, size4_t input_strides,
                   const shared_t<T[]>& output, size4_t output_strides, size4_t shape,
                   float2_t center, float2_t radius, float taper_size, Stream& stream) {
        return rectangle<INVERT>(input, input_strides, output, output_strides, shape,
                                 float3_t{0, center[0], center[1]}, float3_t{1, radius[0], radius[1]},
                                 taper_size, stream);
    }

    /// Returns or applies a cylindrical mask.
    /// \tparam INVERT          Whether the mask should be inverted. If true, everything within the cylinder is removed.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        On the \b host. Array to mask. If nullptr, write mask in \p output.
    /// \param input_strides    BDHW strides, in elements, of \p input.
    /// \param[out] output      On the \b host. Masked array. Can be equal to \p input.
    /// \param output_strides   BDHW strides, in elements, of \p output.
    /// \param shape            BDHW shape of \p input and \p output.
    /// \param center           DHW center of the cylinder, in \p T elements.
    /// \param radius           Radius of the cylinder.
    /// \param length           Length of the cylinder along the depth dimension.
    /// \param taper_size       Width, in elements, of the raised-cosine, including the first zero.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    template<bool INVERT = false, typename T,
             typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void cylinder(const shared_t<T[]>& input, size4_t input_strides,
                  const shared_t<T[]>& output, size4_t output_strides, size4_t shape,
                  float3_t center, float radius, float length, float taper_size, Stream& stream);
}
