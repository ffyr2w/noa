/// \file noa/gpu/cuda/memory/PtrTexture.h
/// \brief Hold a CUDA texture.
/// \author Thomas - ffyr2w
/// \date 05 Jan 2021

#pragma once

#include "noa/common/Definitions.h"
#include "noa/common/traits/BaseTypes.h"
#include "noa/gpu/cuda/Types.h"
#include "noa/gpu/cuda/Exception.h"

// CUDA textures:
//  -   Address mode: How out of range coordinates are handled. This can be specified for each coordinates (although
//                    the current implementation specifies the same mode for all the dimensions. It is either wrap,
//                    mirror, border or clamp (default).
//                    Note: This is ignored for 1D textures since they don't support addressing modes.
//                    Note: mirror and wrap are only supported for normalized coordinates, otherwise, fallback to clamp.
//  -   Filter mode:  Filtering used when fetching. Either point (neighbour) or linear.
//                    Note: The linear mode is only allowed for float types.
//                    Note: This is ignored for 1D textures since they don't perform any interpolation.
//  -   Read mode:    Whether or not integer data should be converted to floating point when fetching. If signed,
//                    returns float within [-1., 1.]. If unsigned, returns float within [0., 1.].
//                    Note: This only applies to 8-bit and 16-bit integer formats. 32-bits are not promoted.
//  -   Normalized coordinates: Whether or not the coordinates are normalized when fetching.
//                              If false (default): textures are fetched using floating point coordinates in range
//                                                  [0, N-1], where N is the size of that particular dimension.
//                              If true:            textures are fetched using floating point coordinates in range
//                                                  [0., 1. -1/N], where N is the size of that particular dimension.
//
// Notes:
//  - Textures are bound to global memory, either through a device pointer or a CUDA array.
//      -- Data can be updated but texture cache is not notified of CUDA array modifications. Start a new kernel to update.
//      -- The device pointer or a CUDA array should not be freed while the texture is being used.
//
// TODO Add the other BorderModes, especially BORDER_VALUE by doing the addressing ourself. This could be a bit annoying
//      since it will need some specialization for the modes that are not supported by CUDA...

namespace noa::cuda::memory {
    /// Manages a 1D, 2D or 3D texture object. This object is not copyable nor movable.
    /// Can be created from CUDA arrays, padded memory (2D only) and linear memory (1D only).
    /// \note   Currently supported components (either 1 or 2 per elements) are either signed or unsigned 8-, 16-, or
    ///         32-bit integers, 16-bit floats, or 32-bit floats.
    template<typename T>
    class PtrTexture {
    private:
        // The type T is within the type of PtrTexture even if it doesn't have to.
        // This is just to make PtrTexture more similar to other Ptr* and it is clearer IMHO.
        static_assert(noa::traits::is_valid_ptr_type_v<T>);
        static_assert(noa::traits::is_int_v<T> || noa::traits::is_float_v<T> || noa::traits::is_complex_v<T>);
        static_assert(!std::is_same_v<T, uint64_t> && !std::is_same_v<T, int64_t> &&
                      !std::is_same_v<T, double> && !std::is_same_v<T, cdouble_t>);

        cudaTextureObject_t m_texture{}; // this is just an integer.
        bool m_is_allocated{}; // m_texture is opaque so this is just to be safe and not rely on 0 being an empty texture.

    public: // Texture utilities
        /// Returns a texture object's texture descriptor.
        static NOA_HOST cudaTextureDesc getDescription(cudaTextureObject_t texture) {
            cudaTextureDesc tex_desc{};
            NOA_THROW_IF(cudaGetTextureObjectTextureDesc(&tex_desc, texture));
            return tex_desc;
        }

        /// Whether or not \p texture is using normalized coordinates.
        static NOA_HOST bool hasNormalizedCoordinates(cudaTextureObject_t texture) {
            return getDescription(texture).normalizedCoords;
        }

        /// Sets the underlying texture filter and coordinate mode according to \p interp and \p border.
        /// \details The interpolation functions in ::noa expect the texture to be set as follow:
        ///             - 1) BORDER_MIRROR and BORDER_PERIODIC requires normalized coordinates.
        ///             - 2) The accurate methods use nearest lookups, while the fast methods use linear lookups.
        ///             - 3) INTERP_NEAREST and INTERP_LINEAR_FAST are the only modes supporting normalized coordinates,
        ///                  thus they are the only modes supporting BORDER_MIRROR and BORDER_PERIODIC.
        ///
        /// \param interp                       Desired interpolation/filter method.
        /// \param border                       Desired border/addressing mode.
        /// \param[out] filter_mode             The filter mode that the texture is expected to have.
        /// \param[out] address_mode            The address mode that the texture is expected to have.
        /// \param[out] normalized_coordinates  Whether or not the texture should have normalized coordinates.
        ///
        /// \throw If \p interp and \p border are incompatible or not supported.
        /// \see transform::tex1D(), transform::tex2D() and transform::tex3D() for more details.
        static NOA_HOST void setDescription(InterpMode interp, BorderMode border,
                                            cudaTextureFilterMode* filter_mode,
                                            cudaTextureAddressMode* address_mode,
                                            bool* normalized_coordinates) {
            switch (interp) {
                case INTERP_NEAREST:
                case INTERP_LINEAR:
                case INTERP_COSINE:
                case INTERP_CUBIC:
                case INTERP_CUBIC_BSPLINE:
                    *filter_mode = cudaFilterModePoint;
                    break;
                case INTERP_LINEAR_FAST:
                case INTERP_COSINE_FAST:
                case INTERP_CUBIC_BSPLINE_FAST:
                    *filter_mode = cudaFilterModeLinear;
                    break;
                default:
                    NOA_THROW("{} is not supported", interp);
            }

            // BorderMode is compatible with cudaTextureAddressMode
            if (border == BORDER_PERIODIC || border == BORDER_MIRROR) {
                *address_mode = static_cast<cudaTextureAddressMode>(border);
                *normalized_coordinates = true;
                if (interp != INTERP_LINEAR_FAST && interp != INTERP_NEAREST)
                    NOA_THROW("{} is not supported with {}", border, interp);
            } else if (border == BORDER_ZERO || border == BORDER_CLAMP) {
                *address_mode = static_cast<cudaTextureAddressMode>(border);
                *normalized_coordinates = false;
            } else {
                NOA_THROW("{} is not supported", border);
            }
        }

    public: // Generic alloc functions
        /// Creates a 1D, 2D or 3D texture from a CUDA array.
        /// \param array                        CUDA array. Its lifetime should exceed the life of this new object.
        ///                                     It is directly passed to the returned texture, i.e. its type doesn't
        ///                                     have to match \p T.
        /// \param interp_mode                  Filter mode, either cudaFilterModePoint or cudaFilterModeLinear.
        /// \param border_mode                  Address mode, either cudaAddressModeWrap, cudaAddressModeClamp,
        ///                                     cudaAddressModeMirror or cudaAddressModeBorder.
        /// \param normalized_coordinates       Whether or not the coordinates are normalized when fetching.
        /// \param normalized_reads_to_float    Whether or not 8-, 16-integer data should be converted to float when fetching.
        ///                                     Either cudaReadModeElementType or cudaReadModeNormalizedFloat.
        ///
        /// \note cudaFilterModePoint is only supported for integers.
        /// \note cudaAddressModeMirror and cudaAddressModeWrap are only available with normalized coordinates.
        ///       If \p normalized_coordinates is false, \p border_mode is switched (internally by CUDA) to cudaAddressModeClamp.
        static NOA_HOST cudaTextureObject_t alloc(const cudaArray* array,
                                                  cudaTextureFilterMode interp_mode,
                                                  cudaTextureAddressMode border_mode,
                                                  cudaTextureReadMode normalized_reads_to_float,
                                                  bool normalized_coordinates) {
            cudaResourceDesc res_desc{};
            res_desc.resType = cudaResourceTypeArray;
            res_desc.res.array.array = const_cast<cudaArray*>(array);
            // cudaArrayGetInfo can be used to extract the array type and make
            // sure it matches T, but is it really useful? Maybe just an assert?

            cudaTextureDesc tex_desc{};
            tex_desc.addressMode[0] = border_mode;
            tex_desc.addressMode[1] = border_mode; // ignored if 1D array.
            tex_desc.addressMode[2] = border_mode; // ignored if 1D or 2D array.
            tex_desc.filterMode = interp_mode;
            tex_desc.readMode = normalized_reads_to_float;
            tex_desc.normalizedCoords = normalized_coordinates;

            cudaTextureObject_t texture;
            if (cudaCreateTextureObject(&texture, &res_desc, &tex_desc, nullptr))
                NOA_THROW("Creating the texture object from a CUDA array failed, "
                          "with normalized={}, filter={}, addressing={}, reads_to_float={}",
                          normalized_coordinates, interp_mode, border_mode, normalized_reads_to_float);
            return texture;
        }

        /// Creates a 2D texture from a padded memory layout.
        /// \param[in] array                    On the \b device. Its lifetime should exceed the life of this new object.
        /// \param pitch                        Pitch, in elements, of \p array.
        /// \param shape                        Physical {fast, medium, slow} shape of \p array.
        /// \param interp_mode                  Filter mode, either cudaFilterModePoint or cudaFilterModeLinear.
        /// \param border_mode                  Address mode, either cudaAddressModeWrap, cudaAddressModeClamp,
        ///                                     cudaAddressModeMirror or cudaAddressModeBorder.
        /// \param normalized_coordinates       Whether or not the coordinates are normalized when fetching.
        /// \param normalized_reads_to_float    Whether or not 8-, 16-integer data should be converted to float when fetching.
        ///                                     Either cudaReadModeElementType or cudaReadModeNormalizedFloat.
        ///
        /// \note Texture bound to pitched linear memory are usually used if conversion to CUDA arrays is too tedious,
        ///       but warps should preferably only access rows, for performance reasons.
        /// \note cudaDeviceProp::textureAlignment is satisfied by cudaMalloc* and cudaDeviceProp::texturePitchAlignment
        ///       is satisfied by cudaMalloc3D/Pitch. Care should be taken about offsets when working on subregions.
        static NOA_HOST cudaTextureObject_t alloc(const T* array, size_t pitch, size3_t shape,
                                                  cudaTextureFilterMode interp_mode,
                                                  cudaTextureAddressMode border_mode,
                                                  cudaTextureReadMode normalized_reads_to_float,
                                                  bool normalized_coordinates) {
            if (shape.z > 1)
                NOA_THROW("Cannot create a 3D texture object from an array of shape {}. Use a CUDA array.", shape);

            cudaResourceDesc res_desc{};
            res_desc.resType = cudaResourceTypePitch2D;
            res_desc.res.pitch2D.devPtr = const_cast<T*>(array);
            res_desc.res.pitch2D.desc = cudaCreateChannelDesc<T>();
            res_desc.res.pitch2D.width = shape.x;
            res_desc.res.pitch2D.height = shape.y;
            res_desc.res.pitch2D.pitchInBytes = pitch * sizeof(T);

            cudaTextureDesc tex_desc{};
            tex_desc.addressMode[0] = border_mode;
            tex_desc.addressMode[1] = border_mode; // addressMode[2] isn't used.
            tex_desc.filterMode = interp_mode;
            tex_desc.readMode = normalized_reads_to_float;
            tex_desc.normalizedCoords = normalized_coordinates;

            cudaTextureObject_t texture;
            if (cudaCreateTextureObject(&texture, &res_desc, &tex_desc, nullptr))
                NOA_THROW("Creating the texture object from a CUDA array failed, "
                          "with normalized={}, filter={}, addressing={}, reads_to_float={}",
                          normalized_coordinates, interp_mode, border_mode, normalized_reads_to_float);
        }

        /// Creates a 1D texture from linear memory.
        /// \param[in] array                    On the \b device. Its lifetime should exceed the life of this new object.
        /// \param elements                     Size, in elements, of \p array.
        /// \param normalized_coordinates       Whether or not the coordinates are normalized when fetching.
        /// \param normalized_reads_to_float    Whether or not 8-, 16-integer data should be converted to float when fetching.
        ///                                     Either cudaReadModeElementType or cudaReadModeNormalizedFloat.
        ///
        /// \note   Textures bound to linear memory only support integer indexing (no interpolation),
        ///         and they don't have addressing modes. They are usually used if the texture cache can assist L1 cache.
        /// \note   cudaDeviceProp::textureAlignment is satisfied by cudaMalloc*.
        ///         Care should be taken about offsets when working on subregions.
        static NOA_HOST cudaTextureObject_t alloc(const T* array, size_t elements,
                                                  cudaTextureReadMode normalized_reads_to_float,
                                                  bool normalized_coordinates) {
            cudaResourceDesc res_desc{};
            res_desc.resType = cudaResourceTypeLinear;
            res_desc.res.linear.devPtr = const_cast<T*>(array);
            res_desc.res.linear.desc = cudaCreateChannelDesc<T>();
            res_desc.res.linear.sizeInBytes = elements * sizeof(T);

            cudaTextureDesc tex_desc{};
            tex_desc.readMode = normalized_reads_to_float;
            tex_desc.normalizedCoords = normalized_coordinates;

            cudaTextureObject_t texture;
            NOA_THROW_IF(cudaCreateTextureObject(&texture, &res_desc, &tex_desc, nullptr));
            return texture;
        }

        static NOA_HOST void dealloc(cudaTextureObject_t texture) {
            NOA_THROW_IF(cudaDestroyTextureObject(texture));
        }

    public: // noa alloc functions
        /// Creates a 1D, 2D or 3D texture from a CUDA array.
        /// \param[in] array    CUDA array. Its lifetime should exceed the life of this new object.
        /// \param interp_mode  Any of InterpMode.
        /// \param border_mode  Either BORDER_ZERO, BORDER_CLAMP, BORDER_PERIODIC or BORDER_MIRROR.
        /// \see PtrTexture<T>::setDescription() for more details.
        static NOA_HOST cudaTextureObject_t alloc(const cudaArray* array,
                                                  InterpMode interp_mode,
                                                  BorderMode border_mode) {
            cudaTextureFilterMode filter;
            cudaTextureAddressMode address;
            bool normalized_coords;
            setDescription(interp_mode, border_mode, &filter, &address, &normalized_coords);
            return alloc(array, filter, address, cudaReadModeElementType, normalized_coords);
        }

        /// Creates a 2D texture from a padded memory layout.
        /// \param[in] array    On the \b device. Its lifetime should exceed the life of this new object.
        /// \param pitch        Pitch, in elements, of \p array.
        /// \param shape        Physical {fast, medium, slow} shape of \p array.
        /// \param interp_mode  Any of InterpMode.
        /// \param border_mode  Either BORDER_ZERO, BORDER_CLAMP, BORDER_PERIODIC or BORDER_MIRROR.
        /// \see PtrTexture<T>::setDescription() for more details.
        static NOA_HOST cudaTextureObject_t alloc(const T* array, size_t pitch, size3_t shape,
                                                  InterpMode interp_mode, BorderMode border_mode) {
            cudaTextureFilterMode filter;
            cudaTextureAddressMode address;
            bool normalized_coords;
            setDescription(interp_mode, border_mode, &filter, &address, &normalized_coords);
            return alloc(array, pitch, shape, filter, address, cudaReadModeElementType, normalized_coords);
        }

    public: // Constructors, destructor
        /// Creates an empty instance. Use reset() to create a new texture.
        PtrTexture() = default;

        /// Creates a 1D, 2D or 3D texture from a CUDA array.
        NOA_HOST PtrTexture(const cudaArray* array,
                            cudaTextureFilterMode interp_mode,
                            cudaTextureAddressMode border_mode,
                            cudaTextureReadMode read_mode,
                            bool normalized_coordinates)
                : m_texture(alloc(array, interp_mode, border_mode, read_mode, normalized_coordinates)),
                  m_is_allocated(true) {}

        /// Creates a 1D, 2D or 3D texture from a CUDA array.
        NOA_HOST PtrTexture(const cudaArray* array, InterpMode interp_mode, BorderMode border_mode)
                : m_texture(alloc(array, interp_mode, border_mode)), m_is_allocated(true) {}

        /// Creates a 2D texture from a padded memory layout.
        NOA_HOST PtrTexture(const T* array, size_t pitch, size3_t shape,
                            cudaTextureFilterMode interp_mode,
                            cudaTextureAddressMode border_mode,
                            cudaTextureReadMode read_mode,
                            bool normalized_coordinates)
                : m_texture(alloc(array, pitch, shape, interp_mode, border_mode, read_mode, normalized_coordinates)),
                  m_is_allocated(true) {}

        /// Creates a 2D texture from a padded memory layout.
        NOA_HOST PtrTexture(const T* array, size_t pitch, size3_t shape, InterpMode interp_mode, BorderMode border_mode)
                : m_texture(alloc(array, pitch, shape, interp_mode, border_mode)), m_is_allocated(true) {}

        /// Creates a 1D texture from linear memory.
        NOA_HOST PtrTexture(const T* array, size_t elements,
                            cudaTextureReadMode read_mode,
                            bool normalized_coordinates)
                : m_texture(alloc(array, elements, read_mode, normalized_coordinates)),
                  m_is_allocated(true) {}

        // For now let's just ignore these possibilities.
        PtrTexture(const PtrTexture& to_copy) = delete;
        PtrTexture& operator=(const PtrTexture& to_copy) = delete;
        PtrTexture(PtrTexture<T>&& to_move) = delete;
        PtrTexture<T>& operator=(PtrTexture<T>&& to_move) = delete;

        NOA_HOST ~PtrTexture() {
            if (m_is_allocated) {
                cudaError_t err = cudaDestroyTextureObject(m_texture);
                if (err != cudaSuccess && std::uncaught_exceptions() == 0)
                    NOA_THROW(toString(err));
            }
        }

    public:
        [[nodiscard]] NOA_HOST constexpr cudaTextureObject_t get() const noexcept { return m_texture; }
        [[nodiscard]] NOA_HOST constexpr cudaTextureObject_t id() const noexcept { return m_texture; }
        [[nodiscard]] NOA_HOST constexpr cudaTextureObject_t data() const noexcept { return m_texture; }

        /// Whether or not the object manages a texture.
        [[nodiscard]] NOA_HOST constexpr bool empty() const noexcept { return !m_is_allocated; }
        [[nodiscard]] NOA_HOST constexpr explicit operator bool() const noexcept { return !empty(); }

        /// Clears the underlying array, if necessary. empty() will evaluate to true.
        NOA_HOST void reset() {
            if (m_is_allocated) {
                dealloc(m_texture);
                m_is_allocated = false;
            }
        }

        /// Clears the underlying array, if necessary. This is identical to reset().
        NOA_HOST void dispose() { reset(); } // dispose might be a better name than reset...

        /// Resets the underlying array. The new data is owned.
        NOA_HOST void reset(const cudaArray* array,
                            cudaTextureFilterMode interp_mode,
                            cudaTextureAddressMode border_mode,
                            cudaTextureReadMode read_mode,
                            bool normalized_coordinates) {
            if (m_is_allocated)
                dealloc(m_texture);
            m_texture = alloc(array, interp_mode, border_mode, read_mode, normalized_coordinates);
            m_is_allocated = true;
        }

        /// Resets the underlying array. The new data is owned.
        NOA_HOST void reset(const T* array, size_t pitch, size3_t shape,
                            cudaTextureFilterMode interp_mode,
                            cudaTextureAddressMode border_mode,
                            cudaTextureReadMode read_mode,
                            bool normalized_coordinates) {
            if (m_is_allocated)
                dealloc(m_texture);
            m_texture = alloc(array, pitch, shape, interp_mode, border_mode, read_mode, normalized_coordinates);
            m_is_allocated = true;
        }

        /// Resets the underlying array. The new data is owned.
        NOA_HOST void reset(const T* array, size_t elements,
                            cudaTextureReadMode read_mode,
                            bool normalized_coordinates) {
            if (m_is_allocated)
                dealloc(m_texture);
            m_texture = alloc(array, elements, read_mode, normalized_coordinates);
            m_is_allocated = true;
        }

        /// Resets the underlying array. The new data is owned.
        NOA_HOST void reset(const cudaArray* array, InterpMode interp_mode, BorderMode border_mode) {
            if (m_is_allocated)
                dealloc(m_texture);
            m_texture = alloc(array, interp_mode, border_mode);
            m_is_allocated = true;
        }

        /// Resets the underlying array. The new data is owned.
        NOA_HOST void reset(const T* array, size_t pitch, size3_t shape,
                            InterpMode interp_mode, BorderMode border_mode) {
            if (m_is_allocated)
                dealloc(m_texture);
            m_texture = alloc(array, pitch, shape, interp_mode, border_mode);
            m_is_allocated = true;
        }

        /// Releases the ownership of the managed array, if any.
        /// In this case, the caller is responsible for deleting the object.
        /// get() returns nullptr after the call and empty() returns true.
        [[nodiscard]] NOA_HOST cudaTextureObject_t release() noexcept {
            m_is_allocated = false;
            return m_texture;
        }
    };
}
