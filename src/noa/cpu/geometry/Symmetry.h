/// \file noa/cpu/geometry/Symmetry.h
/// \brief Symmetry transformations for images and volumes.
/// \author Thomas - ffyr2w
/// \date 04 Sep 2021

#pragma once

#include "noa/common/Definitions.h"
#include "noa/common/Types.h"
#include "noa/common/Exception.h"
#include "noa/common/geometry/Symmetry.h"
#include "noa/cpu/Stream.h"

namespace noa::cpu::geometry {
    using Symmetry = ::noa::geometry::Symmetry;

    /// Symmetrizes the 2D (batched) input array.
    /// \tparam PREFILTER       Whether or not the input should be prefiltered.
    ///                         Only used if \p interp_mode is INTERP_CUBIC_BSPLINE or INTERP_CUBIC_BSPLINE_FAST.
    /// \tparam T               float, double, cfloat, cdouble_t.
    /// \param[in] input        On the \b host. Input array to symmetrize.
    /// \param input_stride     Rightmost stride, in elements, of \p input.
    /// \param[out] output      On the \b host. Symmetrized array.
    /// \param output_stride    Rightmost stride, in elements, of \p output.
    /// \param shape            Rightmost shape, in elements, of \p input and \p output.
    /// \param symmetry         Symmetry operator.
    /// \param center           Rightmost center of the symmetry.
    /// \param interp_mode      Interpolation/filter mode. All interpolation modes are supported.
    /// \param normalize        Whether \p output should be normalized to have the same range as \p input.
    ///                         If false, output values end up being scaled by the symmetry count.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    ///
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \note In-place computation is not allowed, i.e. \p input and \p output should not overlap.
    /// \note During transformation, out-of-bound elements are set to 0, i.e. BORDER_ZERO is used.
    template<bool PREFILTER = true, typename T>
    void symmetrize2D(const shared_t<T[]>& input, size4_t input_stride,
                      const shared_t<T[]>& output, size4_t output_stride,
                      size4_t shape, const Symmetry& symmetry, float2_t center,
                      InterpMode interp_mode, bool normalize, Stream& stream);

    /// Symmetrizes the 3D (batched) input array.
    /// \tparam PREFILTER       Whether or not the input should be prefiltered.
    ///                         Only used if \p interp_mode is INTERP_CUBIC_BSPLINE or INTERP_CUBIC_BSPLINE_FAST.
    /// \tparam T               float, double, cfloat, cdouble_t.
    /// \param[in] input        On the \b host. Input array to symmetrize.
    /// \param input_stride     Rightmost stride, in elements, of \p input.
    /// \param[out] output      On the \b host. Symmetrized array.
    /// \param output_stride    Rightmost stride, in elements, of \p output.
    /// \param shape            Rightmost shape, in elements, of \p input and \p output.
    /// \param[in] symmetry     Symmetry operator.
    /// \param center           Rightmost center of the symmetry.
    /// \param interp_mode      Interpolation/filter mode. All interpolation modes are supported.
    /// \param normalize        Whether \p output should be normalized to have the same range as \p input.
    ///                         If false, output values end up being scaled by the symmetry count.
    /// \param[in,out] stream   Stream on which to enqueue this function.
    ///
    /// \note Depending on the stream, this function may be asynchronous and may return before completion.
    /// \note In-place computation is not allowed, i.e. \p input and \p output should not overlap.
    /// \note During transformation, out-of-bound elements are set to 0, i.e. BORDER_ZERO is used.
    template<bool PREFILTER = true, typename T>
    void symmetrize3D(const shared_t<T[]>& input, size4_t input_stride,
                      const shared_t<T[]>& output, size4_t output_stride,
                      size4_t shape, const Symmetry& symmetry, float3_t center,
                      InterpMode interp_mode, bool normalize, Stream& stream);
}
