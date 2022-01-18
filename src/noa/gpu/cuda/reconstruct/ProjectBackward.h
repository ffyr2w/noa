/// \file noa/gpu/cuda/reconstruct/ProjectBackward.h
/// \brief Backward projections.
/// \author Thomas - ffyr2w
/// \date 15 Sep 2021

#pragma once

#include "noa/common/Definitions.h"
#include "noa/gpu/cuda/Types.h"
#include "noa/gpu/cuda/Stream.h"

namespace noa::cuda::reconstruct {
    /// Adds Fourier "slices" into a Fourier volume using tri-linear interpolation.
    /// \details The projections are phase shifted, their magnification is corrected and the EWS curvature is applied.
    ///          Then, they are rotated and added to the cartesian (oversampled) 3D volume using tri-linear interpolation.
    ///
    /// \tparam IS_PROJ_CENTERED        Whether or not the input \p proj and \p proj_weights are centered.
    ///                                 See "noa/cpu/fourier/Resize.h" for more details on FFT layouts.
    /// \tparam IS_VOLUME_CENTERED      Whether or not the output \p volume and \p volume_weight are centered.
    /// \tparam T                       float or double.
    /// \param[in] proj                 On the \b device. Non-redundant projections to insert. One per projection.
    /// \param[in] proj_weights         On the \b device. Element-wise weights associated to \p proj. One per projection.
    /// \param proj_pitch               Pitch, in elements, of \p proj and \p proj_weights.
    /// \param proj_dim                 Logical dimension size, in elements, of \p proj and \p proj_weights.
    /// \param[out] volume              On the \b device. Non-redundant volume inside which the projections are inserted.
    ///                                 If nullptr, it is ignored, as well as \p proj.
    /// \param[out] volume_weights      On the \b device. Element-wise weights associated to the volume.
    ///                                 If nullptr, it is ignored, as well as \p proj_weights.
    /// \param volume_pitch             Pitch, in elements, of \p volume and \p volume_weights.
    /// \param volume_dim               Logical dimension size, in elements, of \p volume and \p volume_weights.
    ///
    /// \param[in] proj_shifts          On the \b device. If nullptr, it is ignored. One per projection.
    ///                                 2D real-space shifts to apply (as phase-shifts) to the projection before any
    ///                                 other transformation. Positive values shift the real-space object to the right.
    /// \param[in] proj_scaling_factors On the \b host. If nullptr, it is ignored. One per projection.
    ///                                 2D real-space scaling to apply to the projection before the rotation.
    ///                                 The third value is the in-plane scaling angle, in radians.
    /// \param[in] proj_rotations       On the \b host. 3x3 rotation matrices of the projections. One per projection.
    ///                                 The rotation center is on the DC component.
    ///                                 See "noa/common/transform/README.txt" for more details on the conventions.
    /// \param proj_count               Number of projections to insert.
    /// \param freq_max                 Maximum frequency to insert. Values are clamped from 0 (DC) to 0.5 (Nyquist).
    /// \param ewald_sphere_radius      Ewald sphere radius, in 1/pixel (e.g. `pixel_size/wavelength`).
    ///                                 If negative, the negative curve is computed.
    ///                                 If 0.f, the "projections" are assumed to be actual projections.
    /// \param[in,out] stream           Stream on which to enqueue this function.
    ///                                 The stream is synchronized when the function returns.
    ///
    /// \note Since a rotation is applied to the projections, they should have their real-space rotation-center
    ///       phase-shifted at 0, as with any rotation applied in Fourier space. If this is not the case,
    ///       \p proj_shifts can be used to properly phase shift the projection before the rotation.
    /// \note Only square projections and cubic volumes are supported. The volume is usually oversampled compared to
    ///       the projections. The oversampling ratio is set to be the ratio between \p volume_dim and \p proj_dim.
    ///       Note that both the projections and the volume are non-redundant transforms, so the physical size in
    ///       the first dimension is `x/2+1`, x being the logical size, i.e. \p volume_dim or \p proj_dim.
    /// \note In order to have both left and right beams assigned to different values, this function only computes one
    ///       "side" of the EWS, as specified by \p ewald_sphere_radius. To insert the other side, one would have to
    ///       call this function a second time with the negative \p ewald_sphere_radius.
    /// \note For consistency with the CUDA backend and for efficiency, the redundant line at x=0 is entirely inserted
    ///       into the volume. If the projection has an in-plane rotation, this results into having this line inserted
    ///       twice. This shouldn't be a problem, since the \p volume is associated with \p volume_weights and these
    ///       weights are correctly updated.
    template<bool IS_PROJ_CENTERED = false, bool IS_VOLUME_CENTERED = true, typename T>
    NOA_HOST void projectBackward(const Complex<T>* proj, const T* proj_weights, size_t proj_pitch, size_t proj_dim,
                                  Complex<T>* volume, T* volume_weights, size_t volume_pitch, size_t volume_dim,
                                  const float2_t* proj_shifts, const float3_t* proj_scaling_factors,
                                  const float33_t* proj_rotations, uint proj_count,
                                  float freq_max, float ewald_sphere_radius, Stream& stream);

    /// Adds Fourier "slices" into a Fourier volume using tri-linear interpolation.
    /// Same as above, except that the same magnification correction is applied to all projections.
    template<bool IS_PROJ_CENTERED = false, bool IS_VOLUME_CENTERED = true, typename T>
    NOA_HOST void projectBackward(const Complex<T>* proj, const T* proj_weights, size_t proj_pitch, size_t proj_dim,
                                  Complex<T>* volume, T* volume_weights, size_t volume_pitch, size_t volume_dim,
                                  const float2_t* proj_shifts, float3_t proj_scaling_factor,
                                  const float33_t* proj_rotations, uint proj_count,
                                  float freq_max, float ewald_sphere_radius, Stream& stream);
}

// -- Max to Nyquist, flat EWS -- //
namespace noa::cuda::reconstruct{
    /// Adds Fourier "slices" into a Fourier volume using tri-linear interpolation.
    /// Overload which sets the maximum frequency to Nyquist and uses a "flat" Ewald sphere.
    template<bool IS_PROJ_CENTERED = false, bool IS_VOLUME_CENTERED = true, typename T>
    NOA_IH void projectBackward(const Complex<T>* proj, const T* proj_weights, size_t proj_pitch, size_t proj_dim,
                                Complex<T>* volume, T* volume_weights, size_t volume_pitch, size_t volume_dim,
                                const float2_t* proj_shifts, const float3_t* proj_scaling_factors,
                                const float33_t* proj_rotations, uint proj_count,
                                Stream& stream) { // overload instead of default value to keep stream at the end
        projectBackward<IS_PROJ_CENTERED, IS_VOLUME_CENTERED>(
                proj, proj_weights, proj_pitch, proj_dim,
                volume, volume_weights, volume_pitch, volume_dim,
                proj_shifts, proj_scaling_factors, proj_rotations, proj_count, 0.5f, 0.f, stream);
    }

    /// Adds Fourier "slices" into a Fourier volume using tri-linear interpolation.
    /// Overload which sets the same magnification correction for all projections.
    /// Overload which sets the maximum frequency to Nyquist and uses a "flat" Ewald sphere.
    template<bool IS_PROJ_CENTERED = false, bool IS_VOLUME_CENTERED = true, typename T>
    NOA_IH void projectBackward(const Complex<T>* proj, const T* proj_weights, size_t proj_pitch, size_t proj_dim,
                                Complex<T>* volume, T* volume_weights, size_t volume_pitch, size_t volume_dim,
                                const float2_t* proj_shifts, float3_t proj_scaling_factor,
                                const float33_t* proj_rotations, uint proj_count,
                                Stream& stream) {
        projectBackward<IS_PROJ_CENTERED, IS_VOLUME_CENTERED>(
                proj, proj_weights, proj_pitch, proj_dim,
                volume, volume_weights, volume_pitch, volume_dim,
                proj_shifts, proj_scaling_factor, proj_rotations, proj_count, 0.5f, 0.f, stream);
    }
}
