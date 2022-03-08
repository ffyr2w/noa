/// \file noa/gpu/cuda/memory/Index.h
/// \brief Remapping functions.
/// \author Thomas - ffyr2w
/// \date 05 Jan 2021
#pragma once

#include "noa/common/Definitions.h"
#include "noa/common/Functors.h"
#include "noa/gpu/cuda/Types.h"
#include "noa/gpu/cuda/Stream.h"

// -- Using coordinates -- //
namespace noa::cuda::memory {
    /// Extracts one or multiple ND (1 <= N <= 3) subregions at various locations in the input array.
    /// \tparam T                   Any data type.
    /// \param[in] input            On the \b device. Input array to use for the extraction.
    /// \param input_stride         Rightmost strides, in elements, of \p input.
    /// \param input_shape          Rightmost shape of \p input.
    /// \param[out] subregions      On the \b device. Output subregions.
    /// \param subregion_stride     Rightmost strides, in elements, of \p subregions.
    /// \param subregion_shape      Rightmost shape of \p subregions.
    /// \param[in] origins          On the \b host or device. One per batch.
    ///                             Rightmost indexes, defining the origin where to extract subregions from \p input.
    ///                             While usually within the input frame, subregions can be (partially) out-of-bound.
    ///                             The outermost dimension of \p subregion_shape is the batch dimension and sets the
    ///                             number of subregions to extract. Thus, subregions can be up to 3 dimensions.
    /// \param border_mode          Border mode used for out-of-bound conditions. Can be BORDER_NOTHING, BORDER_ZERO,
    ///                             BORDER_VALUE, BORDER_CLAMP, BORDER_MIRROR or BORDER_REFLECT.
    /// \param border_value         Constant value to use for out-of-bound conditions.
    ///                             Only used if \p border_mode is BORDER_VALUE.
    /// \param[in,out] stream       Stream on which to enqueue this function.
    ////
    /// \note \p input and \p subregions should not overlap.
    /// \note This function may be asynchronous relative to the host and may return before completion.
    template<typename T>
    NOA_HOST void extract(const T* input, size4_t input_stride, size4_t input_shape,
                          T* subregions, size4_t subregion_stride, size4_t subregion_shape,
                          const int4_t* origins, BorderMode border_mode, T border_value, Stream& stream);

    /// Inserts into the output array one or multiple ND (1 <= N <= 3) subregions at various locations.
    /// \tparam T                   Any data type.
    /// \param[in] subregions       On the \b device. Subregion(s) to insert into \p output.
    /// \param subregion_stride     Rightmost strides, in elements, of \p subregions.
    /// \param subregion_shape      Rightmost shape \p subregions.
    /// \param[out] output          On the \b device. Output array.
    /// \param output_stride        Rightmost strides, in elements, of \p output.
    /// \param output_shape         Rightmost shape of \p output.
    /// \param[in] origins          On the \b host or device. One per batch.
    ///                             Rightmost indexes, defining the origin where to insert subregions into \p output.
    ///                             While usually within the output frame, subregions can be (partially) out-of-bound.
    ///                             The outermost dimension of \p subregion_shape is the batch dimension and sets the
    ///                             number of subregions to insert. Thus, subregions can be up to 3 dimensions.
    /// \param[in,out] stream       Stream on which to enqueue this function.
    ///
    /// \note \p outputs and \p subregions should not overlap.
    /// \note This function assumes no overlap between subregions. There's no guarantee on the order of insertion.
    /// \note This function may be asynchronous relative to the host and may return before completion.
    template<typename T>
    NOA_HOST void insert(const T* subregions, size4_t subregion_stride, size4_t subregion_shape,
                         T* output, size4_t output_stride, size4_t output_shape,
                         const int4_t* origins, Stream& stream);

    /// Gets the atlas layout (shape + subregion origins).
    /// \param subregion_shape          Rightmost shape of the subregion(s).
    ///                                 The outermost dimension is the number of subregion(s) to place into the atlas.
    /// \param[out] origins             On the \b host. Subregion origin(s), relative to the atlas shape.
    /// \return                         Atlas shape.
    ///
    /// \note The shape of the atlas is not necessary a square. For instance, with 4 subregions the atlas layout
    ///       is `2x2`, but with 5 subregions is goes to `3x2` with one empty region. Subregions are in row-major order.
    /// \note The origin is always 0 for the two outermost dimensions. The function is effectively un-batching the
    ///       2D/3D subregions into a 2D/3D atlas.
    NOA_HOST size4_t atlasLayout(size4_t subregion_shape, int4_t* origins);
}

// -- Using a sequence of linear indexes -- //
namespace noa::cuda::memory {
    /// Extracts elements (and/or indexes) from the input array based on an unary bool operator.
    /// \tparam E               Same as \p T or void.
    ///                         If void, the elements are not returned.
    /// \tparam I               Integral type of the extracted elements' indexes. Either uint32_t, or uint64_t.
    ///                         These indexes are mostly used when the extracted elements needs to be inserted
    ///                         back into the input array. If \p I is void, the indexes are not returned.
    /// \tparam T               (u)int32_t, (u)int64_t, half_t, float, double.
    /// \param[in] input        On the \b device. Input array to extract from.
    /// \param stride           Rightmost strides, in elements, of \p input.
    /// \param shape            Rightmost shape of \p input. The outermost dimension is the batch dimension.
    /// \param unary_op         Unary operation device function object that will be used as criterion to extract elements.
    ///                         Each element is passed through that operator and if the return value evaluates
    ///                         to true, the element is extracted. Supported unary operator: noa::math::logical_not_t.
    /// \param[in,out] stream   Stream on which to enqueue this function. The stream is synchronized once.
    /// \return                 1: Extracted elements. If \p E is void, returns nullptr.
    ///                         2: Sequence of indexes. If \p I is void, returns nullptr.
    ///                         3: Number of extracted elements.
    /// \note The caller is the owner of the returned pointers. These pointers should be freed using the asynchronous
    ///       version of PtrDevice::dealloc() (effectively calling cudaFreeAsync()) with \p stream as second argument.
    ///       This implies that either \p stream should outlive the pointers or that the synchronous version of
    ///       PtrDevice::dealloc() (effectively calling cudaFree()) should be called once all work is done.
    /// \note This function may be asynchronous relative to the host and may return before completion.
    template<typename E, typename I = void, typename T, typename UnaryOp>
    NOA_HOST std::tuple<E*, I*, size_t> extract(const T* input, size4_t stride, size4_t shape,
                                                UnaryOp unary_op, Stream& stream);

    /// Extracts elements (and/or indexes) from the input array based on an binary bool operator.
    /// \tparam E               Same as \p T or void.
    ///                         If void, the elements are not returned.
    /// \tparam I               Integral type of the extracted elements' indexes. Either uint32_t, or uint64_t.
    ///                         These indexes are mostly used when the extracted elements needs to be inserted
    ///                         back into the input array. If \p I is void, the indexes are not returned.
    /// \tparam T               (u)int32_t, (u)int64_t, half_t, float, double.
    /// \tparam U               Should be equal to \p T.
    /// \param[in] input        On the \b device. Input array to extract from.
    /// \param stride           Rightmost strides, in elements, of \p input.
    /// \param shape            Rightmost shape of \p input.
    /// \param value            Value to use as right-hand side argument.
    /// \param binary_op        Binary operation function object that will be used as criterion to extract elements.
    ///                         Each element and \p value are passed through that operator and if the return value
    ///                         evaluates to true, the element is extracted.
    ///                         Supported noa::math binary operator: equal_t, not_equal_t, less_t, less_equal_t,
    ///                         greater_t, greater_equal_t.
    /// \param[in,out] stream   Stream on which to enqueue this function. The stream is synchronized once.
    /// \return                 1: Extracted elements. If \p E is void, returns nullptr.
    ///                         2: Sequence of indexes. If \p I is void, returns nullptr.
    ///                         3: Number of extracted elements.
    /// \note The caller is the owner of the returned pointers. These pointers should be freed using the asynchronous
    ///       version of PtrDevice::dealloc() (effectively calling cudaFreeAsync()) with \p stream as second argument.
    ///       This implies that either \p stream should outlive the pointers or that the synchronous version of
    ///       PtrDevice::dealloc() (effectively calling cudaFree()) should be called once all work is done.
    /// \note This function may be asynchronous relative to the host and may return before completion.
    template<typename E, typename I = void, typename T, typename U, typename BinaryOp,
             typename = std::enable_if_t<!std::is_pointer_v<U>>>
    NOA_HOST std::tuple<E*, I*, size_t> extract(const T* input, size4_t stride, size4_t shape, U value,
                                                BinaryOp binary_op, Stream& stream);

    /// Extracts elements (and/or indexes) from the input array based on an binary bool operator.
    /// \tparam E               Same as \p T or void.
    ///                         If void, the elements are not returned.
    /// \tparam I               Integral type of the extracted elements' indexes. Either uint32_t, or uint64_t.
    ///                         These indexes are mostly used when the extracted elements needs to be inserted
    ///                         back into the input array. If \p I is void, the indexes are not returned.
    /// \tparam T               (u)int32_t, (u)int64_t, half_t, float, double.
    /// \tparam U               Should be equal to \p T.
    /// \param[in] input        On the \b device. Input array to extract from.
    /// \param stride           Rightmost strides, in elements, of \p input.
    /// \param shape            Rightmost shape of \p input. The outermost dimension is the batch dimension.
    /// \param[in] values       On the \b host or \b device. Value(s) to use as right-hand side argument. One per batch.
    /// \param binary_op        Binary operation function object that will be used as criterion to extract elements.
    ///                         Each element and the current value are passed through that operator and if the return
    ///                         value evaluates to true, the element is extracted.
    ///                         Supported noa::math binary operator: equal_t, not_equal_t, less_t, less_equal_t,
    ///                         greater_t, greater_equal_t.
    /// \param[in,out] stream   Stream on which to enqueue this function. The stream is synchronized once.
    /// \return                 1: Extracted elements. If \p E is void, returns nullptr.
    ///                         2: Sequence of indexes. If \p I is void, returns nullptr.
    ///                         3: Number of extracted elements.
    /// \note The caller is the owner of the returned pointers. These pointers should be freed using the asynchronous
    ///       version of PtrDevice::dealloc() (effectively calling cudaFreeAsync()) with \p stream as second argument.
    ///       This implies that either \p stream should outlive the pointers or that the synchronous version of
    ///       PtrDevice::dealloc() (effectively calling cudaFree()) should be called once all work is done.
    /// \note This function may be asynchronous relative to the host and may return before completion.
    template<typename E, typename I = void, typename T, typename U, typename BinaryOp>
    NOA_HOST std::tuple<E*, I*, size_t> extract(const T* input, size4_t stride, size4_t shape, const U* values,
                                                BinaryOp binary_op, Stream& stream);

    /// Extracts elements (and/or indexes) from the input array based on an binary bool operator.
    /// \tparam E               Same as \p T or void.
    ///                         If void, the elements are not returned.
    /// \tparam I               Integral type of the extracted elements' indexes. Either uint32_t, or uint64_t.
    ///                         These indexes are mostly used when the extracted elements needs to be inserted
    ///                         back into the input array. If \p I is void, the indexes are not returned.
    /// \tparam T               (u)int32_t, (u)int64_t, half_t, float, double.
    /// \tparam U               Should be equal to \p T.
    /// \param[in] input        On the \b device. Input array to extract from.
    /// \param input_stride     Rightmost strides, in elements, of \p input.
    /// \param[in] array        On the \b device. Array to use as right-hand side argument.
    /// \param array_stride     Rightmost strides, in elements, of \p array.
    /// \param shape            Rightmost shape of \p input. The outermost dimension is the batch dimension.
    /// \param binary_op        Binary operation function object that will be used as criterion to extract elements.
    ///                         Each element of both \p input and \p array are passed through that operator and if the
    ///                         return value evaluates to true, the element is extracted.
    ///                         Supported noa::math binary operator: equal_t, not_equal_t, less_t, less_equal_t,
    ///                         greater_t, greater_equal_t.
    /// \param[in,out] stream   Stream on which to enqueue this function. The stream is synchronized once.
    /// \return                 1: Extracted elements. If \p E is void, returns nullptr.
    ///                         2: Sequence of indexes. If \p I is void, returns nullptr.
    ///                         3: Number of extracted elements.
    /// \note The caller is the owner of the returned pointers. These pointers should be freed using the asynchronous
    ///       version of PtrDevice::dealloc() (effectively calling cudaFreeAsync()) with \p stream as second argument.
    ///       This implies that either \p stream should outlive the pointers or that the synchronous version of
    ///       PtrDevice::dealloc() (effectively calling cudaFree()) should be called once all work is done.
    /// \note This function may be asynchronous relative to the host and may return before completion.
    template<typename E, typename I = void, typename T, typename U, typename BinaryOp>
    NOA_HOST std::tuple<E*, I*, size_t> extract(const T* input, size4_t input_stride,
                                                const U* array, size4_t array_stride,
                                                size4_t shape, BinaryOp binary_op, Stream& stream);

    /// Inserts elements into \p output.
    /// \tparam E                   Same as \p T.
    /// \tparam I                   uint32_t, uint64_t.
    /// \tparam T                   (u)int32_t, (u)int64_t, half_t, float, double.
    /// \param[in] sequence_values  On the \b device. Sequence of values that were extracted and need to be reinserted.
    /// \param[in] sequence_indexes On the \b device. Linear indexes in \p output where the values should be inserted.
    /// \param sequence_size        Number of elements to insert.
    /// \param[out] output          On the \b device. Output array inside which the values are going to be inserted.
    /// \param[in,out] stream       Stream on which to enqueue this function.
    /// \note This function may be asynchronous relative to the host and may return before completion.
    template<typename E, typename I, typename T>
    NOA_HOST void insert(const E* sequence_values, const I* sequence_indexes, size_t sequence_size,
                         T* output, Stream& stream);
}
