#pragma once

#include "noa/unified/Array.h"

namespace noa::signal {
    /// Returns or applies a spherical mask.
    /// \tparam INVERT      Whether the mask should be inverted. If true, everything within the sphere is removed.
    /// \tparam T           half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input    Array to mask. If empty, write the mask in \p output.
    /// \param[out] output  Masked array. Can be equal to \p input.
    /// \param center       Rightmost center of the sphere.
    /// \param radius       Radius, in elements, of the sphere.
    /// \param taper_size   Width, in elements, of the raised-cosine, including the first zero.
    template<bool INVERT = false, typename T,
             typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void sphere(const Array<T>& input, const Array<T>& output,
                float3_t center, float radius, float taper_size);

    /// Returns or applies a 2D spherical mask.
    /// \tparam INVERT      Whether the mask should be inverted. If true, everything within the sphere is removed.
    /// \tparam T           half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input    Array to mask. If empty, write the mask in \p output.
    /// \param[out] output  Masked array. Can be equal to \p input.
    /// \param center       Rightmost center of the sphere.
    /// \param radius       Radius, in elements, of the sphere.
    /// \param taper_size   Width, in elements, of the raised-cosine, including the first zero.
    template<bool INVERT = false, typename T,
             typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void sphere(const Array<T>& input, const Array<T>& output,
                float2_t center, float radius, float taper_size);

    /// Returns or applies a rectangular mask.
    /// \tparam INVERT      Whether the mask should be inverted. If true, everything within the rectangle is removed.
    /// \tparam T           half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input    Array to mask. If empty, write the mask in \p output.
    /// \param[out] output  Masked array. Can be equal to \p input.
    /// \param center       Rightmost center of the rectangle.
    /// \param radius       Rightmost radius, in elements, of the rectangle.
    /// \param taper_size   Width, in elements, of the raised-cosine, including the first zero.
    template<bool INVERT = false, typename T,
             typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void rectangle(const Array<T>& input, const Array<T>& output,
                   float3_t center, float3_t radius, float taper_size);

    /// Returns or applies a 2D rectangular mask.
    /// \tparam INVERT      Whether the mask should be inverted. If true, everything within the rectangle is removed.
    /// \tparam T           half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input    Array to mask. If empty, write the mask in \p output.
    /// \param[out] output  Masked array. Can be equal to \p input.
    /// \param center       Rightmost center of the rectangle.
    /// \param radius       Rightmost radius, in elements, of the rectangle.
    /// \param taper_size   Width, in elements, of the raised-cosine, including the first zero.
    template<bool INVERT = false, typename T,
             typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void rectangle(const Array<T>& input, const Array<T>& output,
                   float2_t center, float2_t radius, float taper_size);

    /// Returns or applies a cylindrical mask.
    /// \tparam INVERT      Whether the mask should be inverted. If true, everything within the cylinder is removed.
    /// \tparam T           half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input    Array to mask. If empty, write the mask in \p output.
    /// \param[out] output  Masked array. Can be equal to \p input.
    /// \param center       Rightmost center of the cylinder, in \p T elements.
    /// \param radius       Radius of the cylinder.
    /// \param length       Length of the cylinder along the third-most dimension.
    /// \param taper_size   Width, in elements, of the raised-cosine, including the first zero.
    template<bool INVERT = false, typename T,
             typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void cylinder(const Array<T>& input, const Array<T>& output,
                  float3_t center, float radius, float length, float taper_size);
}

#define NOA_UNIFIED_SHAPE_
#include "noa/unified/signal/Shape.inl"
#undef NOA_UNIFIED_SHAPE_
