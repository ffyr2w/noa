#pragma once

#include "noa/unified/Array.h"

namespace noa::signal {
    /// Returns or applies an elliptical mask.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        2D or 3D array(s) to mask. If empty, write the mask in \p output.
    /// \param[out] output      Masked array(s). Can be equal to \p input.
    /// \param center           DHW center of the ellipse.
    /// \param radius           DHW radius, in elements, of the ellipse.
    /// \param edge_size        Width, in elements, of the raised-cosine, including the first zero.
    /// \param inv_transform    Inverse HW matrix to apply on the ellipse. The rotation center is \p center.
    /// \param invert           Whether the mask should be inverted. If true, everything within the ellipse is removed.
    template<typename T, typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void ellipse(const Array<T>& input, const Array<T>& output,
                 float3_t center, float3_t radius, float edge_size,
                 float33_t inv_transform = {}, bool invert = false);

    /// Returns or applies an 2D elliptical mask.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        2D array(s) to mask. If empty, write the mask in \p output.
    /// \param[out] output      Masked array(s). Can be equal to \p input.
    /// \param center           HW center of the ellipse.
    /// \param radius           HW radius, in elements, of the ellipse.
    /// \param edge_size        Width, in elements, of the raised-cosine, including the first zero.
    /// \param inv_transform    Inverse HW matrix to apply on the ellipse. The rotation center is \p center.
    /// \param invert           Whether the mask should be inverted. If true, everything within the ellipse is removed.
    template<typename T, typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void ellipse(const Array<T>& input, const Array<T>& output,
                 float2_t center, float2_t radius, float edge_size,
                 float22_t inv_transform = {}, bool invert = false);

    /// Returns or applies a spherical mask.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        2D or 3D array(s) to mask. If empty, write the mask in \p output.
    /// \param[out] output      Masked array(s). Can be equal to \p input.
    /// \param center           DHW center of the sphere.
    /// \param radius           Radius, in elements, of the sphere.
    /// \param edge_size        Width, in elements, of the raised-cosine, including the first zero.
    /// \param inv_transform    Inverse HW matrix to apply on the sphere. The rotation center is \p center.
    /// \param invert           Whether the mask should be inverted. If true, everything within the sphere is removed.
    template<typename T, typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void sphere(const Array<T>& input, const Array<T>& output,
                float3_t center, float radius, float edge_size,
                float33_t inv_transform = {}, bool invert = false);

    /// Returns or applies a 2D spherical mask.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        2D array(s) to mask. If empty, write the mask in \p output.
    /// \param[out] output      Masked array(s). Can be equal to \p input.
    /// \param center           HW center of the sphere.
    /// \param radius           Radius, in elements, of the sphere.
    /// \param edge_size        Width, in elements, of the raised-cosine, including the first zero.
    /// \param inv_transform    Inverse HW matrix to apply on the sphere. The rotation center is \p center.
    /// \param invert           Whether the mask should be inverted. If true, everything within the sphere is removed.
    template<typename T, typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void sphere(const Array<T>& input, const Array<T>& output,
                float2_t center, float radius, float edge_size,
                float22_t inv_transform = {}, bool invert = false);

    /// Returns or applies a rectangular mask.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        2D or 3D array(s) to mask. If empty, write the mask in \p output.
    /// \param[out] output      Masked array(s). Can be equal to \p input.
    /// \param center           DHW center of the rectangle.
    /// \param radius           DHW radius, in elements, of the rectangle.
    /// \param edge_size        Width, in elements, of the raised-cosine, including the first zero.
    /// \param inv_transform    Inverse HW matrix to apply on the rectangle. The rotation center is \p center.
    /// \param invert           Whether the mask should be inverted. If true, everything within the sphere is removed.
    template<typename T, typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void rectangle(const Array<T>& input, const Array<T>& output,
                   float3_t center, float3_t radius, float edge_size,
                   float33_t inv_transform = {}, bool invert = false);

    /// Returns or applies a 2D rectangular mask.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        2D array(s) to mask. If empty, write the mask in \p output.
    /// \param[out] output      Masked array(s). Can be equal to \p input.
    /// \param center           HW center of the rectangle.
    /// \param radius           HW radius, in elements, of the rectangle.
    /// \param edge_size        Width, in elements, of the raised-cosine, including the first zero.
    /// \param inv_transform    Inverse HW matrix to apply on the rectangle. The rotation center is \p center.
    /// \param invert           Whether the mask should be inverted. If true, everything within the sphere is removed.
    template<typename T, typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void rectangle(const Array<T>& input, const Array<T>& output,
                   float2_t center, float2_t radius, float edge_size,
                   float22_t inv_transform = {}, bool invert = false);

    /// Returns or applies a cylindrical mask.
    /// \tparam T               half_t, float, double, chalf_t, cfloat_t, cdouble_t.
    /// \param[in] input        2D or 3D array(s) to mask. If empty, write the mask in \p output.
    /// \param[out] output      Masked array(s). Can be equal to \p input.
    /// \param center           DHW center of the cylinder, in \p T elements.
    /// \param radius           Radius of the cylinder.
    /// \param length           Length of the cylinder along the depth dimension.
    /// \param edge_size        Width, in elements, of the raised-cosine, including the first zero.
    /// \param inv_transform    Inverse HW matrix to apply on the cylinder. The rotation center is \p center.
    /// \param invert           Whether the mask should be inverted. If true, everything within the sphere is removed.
    template<typename T, typename = std::enable_if_t<traits::is_float_v<T> || traits::is_complex_v<T>>>
    void cylinder(const Array<T>& input, const Array<T>& output,
                  float3_t center, float radius, float length, float edge_size,
                  float33_t inv_transform = {}, bool invert = false);
}

#define NOA_UNIFIED_SHAPE_
#include "noa/unified/signal/Shape.inl"
#undef NOA_UNIFIED_SHAPE_
