/// \file noa/common/types/Indexing.h
/// \brief Indexing utilities.
/// \author Thomas - ffyr2w
/// \date 10 Dec 2020

#pragma once

#include "noa/common/Definitions.h"
#include "noa/common/Exception.h"
#include "noa/common/types/Constants.h"
#include "noa/common/types/Int2.h"
#include "noa/common/types/Int3.h"
#include "noa/common/types/Int4.h"
#include "noa/common/types/Float2.h"
#include "noa/common/types/Float3.h"
#include "noa/common/types/Float4.h"
#include "noa/common/types/ClampCast.h"
#include "noa/common/types/SafeCast.h"

namespace noa::indexing {
    /// Returns the memory offset corresponding to the given 4D indexes.
    /// \param i0,i1,i2,i3  Multi-dimensional indexes.
    /// \param strides      Strides associated with these indexes.
    /// \note In Debug mode, assertions are added to check for negative indexes
    ///       and if the cast to the offset type is bound safe. However, it doesn't
    ///       check for integer overflow.
    template<typename I0, typename I1, typename I2, typename I3, typename S,
             typename std::enable_if_t<traits::is_int_v<I0> && traits::is_int_v<I1> &&
                                       traits::is_int_v<I2> && traits::is_int_v<I3>, bool> = true>
    NOA_FHD constexpr auto at(I0 i0, I1 i1, I2 i2, I3 i3, const Int4<S>& strides) noexcept {
        static_assert(sizeof(S) >= 4, "don't compute memory offsets with less than 4 bytes values...");

        NOA_ASSERT((traits::is_uint_v<I0> || i0 >= I0{0}) &&
                   (traits::is_uint_v<I1> || i1 >= I1{0}) &&
                   (traits::is_uint_v<I2> || i2 >= I2{0}) &&
                   (traits::is_uint_v<I3> || i3 >= I3{0}));
        NOA_ASSERT(isSafeCast<S>(i0) && isSafeCast<S>(i1) &&
                   isSafeCast<S>(i2) && isSafeCast<S>(i3));

        return static_cast<S>(i0) * strides[0] +
               static_cast<S>(i1) * strides[1] +
               static_cast<S>(i2) * strides[2] +
               static_cast<S>(i3) * strides[3];
    }

    /// Returns the memory offset corresponding to the given 4D indexes.
    /// \param index        Multi-dimensional indexes.
    /// \param strides      Strides associated with these indexes.
    /// \note In Debug mode, assertions are added to check for negative indexes
    ///       and if the cast to the offset type is bound safe. However, it doesn't
    ///       check for integer overflow.
    template<typename T, typename U>
    NOA_FHD constexpr auto at(const Int4<T>& index, const Int4<U>& strides) noexcept {
        return at(index[0], index[1], index[2], index[3], strides);
    }

    /// Returns the memory offset corresponding to the given 3D indexes.
    /// \tparam S           Int3 or Int4.
    /// \param i0,i1,i2     Multi-dimensional indexes.
    /// \param strides      Strides associated with these indexes. Only the first 3 values are used.
    /// \note In Debug mode, assertions are added to check for negative indexes
    ///       and if the cast to the offset type is bound safe. However, it doesn't
    ///       check for integer overflow.
    template<typename I0, typename I1, typename I2, typename S,
             typename std::enable_if_t<traits::is_int_v<I0> && traits::is_int_v<I1> && traits::is_int_v<I2> &&
                                       (traits::is_int4_v<S> || traits::is_int3_v<S>), bool> = true>
    NOA_FHD constexpr auto at(I0 i0, I1 i1, I2 i2, const S& strides) noexcept {
        using out_t = traits::value_type_t<S>;
        static_assert(sizeof(out_t) >= 4, "don't compute memory offsets with less than 4 bytes values...");

        NOA_ASSERT((traits::is_uint_v<I0> || i0 >= I0{0}) &&
                   (traits::is_uint_v<I1> || i1 >= I1{0}) &&
                   (traits::is_uint_v<I2> || i2 >= I2{0}));
        NOA_ASSERT(isSafeCast<out_t>(i0) && isSafeCast<out_t>(i1) && isSafeCast<out_t>(i2));

        return static_cast<out_t>(i0) * strides[0] +
               static_cast<out_t>(i1) * strides[1] +
               static_cast<out_t>(i2) * strides[2];
    }

    /// Returns the memory offset corresponding to the given 3D indexes.
    /// \tparam S           Int3 or Int4.
    /// \param index        Multi-dimensional indexes.
    /// \param strides      Strides associated with these indexes. Only the first 3 values are used.
    /// \note In Debug mode, assertions are added to check for negative indexes
    ///       and if the cast to the offset type is bound safe. However, it doesn't
    ///       check for integer overflow.
    template<typename I, typename S,
             typename std::enable_if_t<traits::is_int4_v<S> || traits::is_int3_v<S>, bool> = true>
    NOA_FHD constexpr auto at(const Int3<I>& index, const S& strides) noexcept {
        return at(index[0], index[1], index[2], strides);
    }

    /// Returns the memory offset corresponding to the given 2D indexes.
    /// \tparam S           Int2, Int3, or Int4.
    /// \param i0,i1        Multi-dimensional indexes.
    /// \param strides      Strides associated with these indexes. Only the first 2 values are used.
    /// \note In Debug mode, assertions are added to check for negative indexes
    ///       and if the cast to the offset type is bound safe. However, it doesn't
    ///       check for integer overflow.
    template<typename I0, typename I1, typename S,
             typename std::enable_if_t<traits::is_int_v<I0> && traits::is_int_v<I1> && traits::is_intX_v<S>, bool> = true>
    NOA_FHD constexpr auto at(I0 i0, I1 i1, const S& strides) noexcept {
        using out_t = traits::value_type_t<S>;
        static_assert(sizeof(out_t) >= 4, "don't compute memory offsets with less than 4 bytes values...");

        NOA_ASSERT((traits::is_uint_v<I0> || i0 >= I0{0}) &&
                   (traits::is_uint_v<I1> || i1 >= I1{0}));
        NOA_ASSERT(isSafeCast<out_t>(i0) && isSafeCast<out_t>(i1));

        return static_cast<out_t>(i0) * strides[0]+
               static_cast<out_t>(i1) * strides[1];
    }

    /// Returns the memory offset corresponding to the given 2D indexes.
    /// \tparam S           Int2, Int3, or Int4.
    /// \param i0,i1        Multi-dimensional indexes.
    /// \param strides      Strides associated with these indexes. Only the first 2 values are used.
    /// \note In Debug mode, assertions are added to check for negative indexes
    ///       and if the cast to the offset type is bound safe. However, it doesn't
    ///       check for integer overflow.
    template<typename I, typename S,
             typename std::enable_if_t<traits::is_int_v<I> && traits::is_intX_v<S>, bool> = true>
    NOA_FHD constexpr auto at(const Int2<I>& index, const S& strides) noexcept {
        return at(index[0], index[1], strides);
    }

    /// Returns the memory offset corresponding to the given 1D indexes.
    /// \tparam S           Integer, Int2, Int3, or Int4.
    /// \param i0           Index.
    /// \param strides      Strides associated with these indexes. Only the first value is used.
    /// \note In Debug mode, assertions are added to check for negative indexes
    ///       and if the cast to the offset type is bound safe. However, it doesn't
    ///       check for integer overflow.
    template<typename I, typename S,
             typename std::enable_if_t<traits::is_int_v<I> && (traits::is_intX_v<S> || traits::is_int_v<S>), bool> = true>
    NOA_FHD constexpr auto at(I i0, S strides) noexcept {
        using out_t = traits::value_type_t<S>;
        static_assert(sizeof(out_t) >= 4, "don't compute memory offsets with less than 4 bytes values...");

        NOA_ASSERT(traits::is_uint_v<I> || i0 >= I{0});
        NOA_ASSERT(isSafeCast<out_t>(i0));

        if constexpr (traits::is_int_v<S>) {
            return static_cast<out_t>(i0) * strides;
        } else {
            return static_cast<out_t>(i0) * strides[0];
        }
    }

    /// If \p idx is out-of-bound, computes a valid index, i.e. [0, size-1], according to \p MODE.
    /// Otherwise, returns \p idx. \p size should be > 0.
    template<BorderMode MODE, typename T, typename = std::enable_if_t<std::is_signed_v<T>>>
    NOA_IHD T at(T idx, T size) {
        static_assert(MODE == BORDER_CLAMP || MODE == BORDER_PERIODIC ||
                      MODE == BORDER_MIRROR || MODE == BORDER_REFLECT);
        NOA_ASSERT(size > 0);

        // a % b == a - b * (a / b) == a + b * (-a / b)
        // Having a < 0 is well-defined since C++11.
        if constexpr (MODE == BORDER_CLAMP) {
            if (idx < 0)
                idx = 0;
            else if (idx >= size)
                idx = size - 1;
        } else if constexpr (MODE == BORDER_PERIODIC) {
            // 0 1 2 3 0 1 2 3 0 1 2 3 |  0 1 2 3  | 0 1 2 3 0 1 2 3 0 1 2 3
            T rem = idx % size; // FIXME maybe enclose this, at the expense of two jumps?
            idx = rem < 0 ? rem + size : rem;
        } else if constexpr (MODE == BORDER_MIRROR) {
            // 0 1 2 3 3 2 1 0 0 1 2 3 3 2 1 0 |  0 1 2 3  | 3 2 1 0 0 1 2 3 3 2 1 0
            if (idx < 0)
                idx = -idx - 1;
            if (idx >= size) {
                T period = 2 * size;
                idx %= period;
                if (idx >= size)
                    idx = period - idx - 1;
            }
        } else if constexpr (MODE == BORDER_REFLECT) {
            // 0 1 2 3 2 1 0 1 2 3 2 1 |  0 1 2 3  | 2 1 0 1 2 3 2 1 0
            if (idx < 0)
                idx = -idx;
            if (idx >= size) {
                T period = 2 * size - 2;
                idx %= period;
                if (idx >= size)
                    idx = period - idx;
            }
        }
        return idx;
    }
}

namespace noa::indexing {
    /// Whether \p strides is in the rightmost order.
    /// Rightmost order is when the innermost stride is on the right, and strides increase right-to-left.
    template<typename T, typename = std::enable_if_t<traits::is_intX_v<T>>>
    NOA_FHD constexpr bool isRightmost(T&& strides) {
        for (size_t i = 0; i < std::decay_t<T>::COUNT - 1; ++i)
            if (strides[i] < strides[i + 1])
                return false;
        return true;
    }

    /// Whether \p strides and \p shape describe a contiguous array.
    /// \tparam ORDER 'C' for C-contiguous (row-major) or 'F' for F-contiguous (column-major).
    /// \note Empty dimensions are contiguous by definition since their strides are not used.
    template<char ORDER = 'C', typename T>
    NOA_FHD constexpr bool areContiguous(Int4<T> strides, Int4<T> shape) {
        if (any(shape == 0)) // guard against empty array
            return false;

        if constexpr (ORDER == 'c' || ORDER == 'C') {
            return (shape[0] == 1 || strides[0] == shape[3] * shape[2] * shape[1]) &&
                   (shape[1] == 1 || strides[1] == shape[3] * shape[2]) &&
                   (shape[2] == 1 || strides[2] == shape[3]) &&
                   (shape[3] == 1 || strides[3] == 1);

        } else if constexpr (ORDER == 'f' || ORDER == 'F') {
            return (shape[0] == 1 || strides[0] == shape[3] * shape[2] * shape[1]) &&
                   (shape[1] == 1 || strides[1] == shape[3] * shape[2]) &&
                   (shape[2] == 1 || strides[2] == 1) &&
                   (shape[3] == 1 || strides[3] == shape[2]);
        } else {
            static_assert(traits::always_false_v<T>);
        }
    }

    /// For each dimension, check if it is contiguous.
    /// \details If one wants to know in which dimension the contiguity is broken or if the contiguity is only
    ///          required in a particular dimension, this function can be useful. It supports broadcasting and
    ///          empty dimensions. It is also guarded against empty shapes.
    /// \tparam ORDER 'C' for C-contiguous (row-major) or 'F' for F-contiguous (column-major).
    /// \note If one want to check whether the array is contiguous or not, while all(isContiguous(...)) is equal to
    ///       areContiguous(...), the later is preferred simply because it is clearer and slightly more efficient.
    template<char ORDER = 'C', typename T>
    NOA_FHD constexpr auto isContiguous(Int4<T> strides, Int4<T> shape) {
        if (any(shape == 0)) // guard against empty array
            return bool4_t{0, 0, 0, 0};

        // The next part of the function expects empty dimensions to have a stride of 0
        strides *= Int4<T>(shape != 1);

        if constexpr (ORDER == 'c' || ORDER == 'C') {
            // If dimension is broadcast (or empty), go up one dimension.
            T corrected_stride_2 = strides[3] ? shape[3] * strides[3] : 1;
            T corrected_stride_1 = strides[2] ? shape[2] * strides[2] : corrected_stride_2;
            T corrected_stride_0 = strides[1] ? shape[1] * strides[1] : corrected_stride_1;

            // If dimension is empty, it is by definition contiguous and its stride does not matter.
            return bool4_t{shape[0] == 1 || strides[0] == corrected_stride_0,
                           shape[1] == 1 || strides[1] == corrected_stride_1,
                           shape[2] == 1 || strides[2] == corrected_stride_2,
                           shape[3] == 1 || strides[3] == 1};

        } else if constexpr (ORDER == 'f' || ORDER == 'F') {
            // If dimension is broadcast (or empty), go up one dimension.
            T corrected_stride_3 = strides[2] ? shape[2] * strides[2] : 1;
            T corrected_stride_1 = strides[3] ? shape[3] * strides[3] : corrected_stride_3;
            T corrected_stride_0 = strides[1] ? shape[1] * strides[1] : corrected_stride_1;

            // If dimension is empty, it is by definition contiguous and its stride does not matter.
            return bool4_t{shape[0] == 1 || strides[0] == corrected_stride_0,
                           shape[1] == 1 || strides[1] == corrected_stride_1,
                           shape[2] == 1 || strides[2] == 1,
                           shape[3] == 1 || strides[3] == corrected_stride_3};

        } else {
            static_assert(traits::always_false_v<T>);
        }
    }

    /// Whether or not a shape describes vector.
    /// \details A vector has one dimension with a size >= 1 and all of the other dimensions empty (i.e. size == 1).
    ///          By this definition, the shapes {1,1,1,1}, {5,1,1,1} and {1,1,1,5} are all vectors.
    ///          If \p can_be_batched is true, the shape can describe a batch of vectors. For instance {4,1,1,5} is
    ///          describing 4 row vectors with a length of 5.
    template<typename T>
    NOA_FHD constexpr bool isVector(Int4<T> shape, bool can_be_batched = false) {
        int non_empty_dimension = 0;
        for (int i = 0; i < 4; ++i) {
            if (shape[i] == 0)
                return false; // empty/invalid shape
            if (!(can_be_batched && i == 0) && shape[i] > 1)
                ++non_empty_dimension;
        }
        return non_empty_dimension <= 1;
    }

    /// Whether or not a shape describes vector.
    /// \details A vector has one dimension with a size >= 1 and all of the other dimensions empty (i.e. size == 1).
    ///          By this definition, the shapes {1,1,1}, {5,1,1} and {1,1,5} are all vectors.
    template<typename T>
    NOA_FHD constexpr bool isVector(Int3<T> shape) {
        int non_empty_dimension = 0;
        for (int i = 0; i < 3; ++i) {
            if (shape[i] == 0)
                return false; // empty/invalid shape
            if (shape[i] > 1)
                ++non_empty_dimension;
        }
        return non_empty_dimension <= 1;
    }

    /// Returns the effective shape: if a dimension has a stride of 0, the effective size is 1 (empty dimension).
    template<typename T, typename = std::enable_if_t<traits::is_intX_v<T>>>
    NOA_IH T effectiveShape(T shape, T strides) noexcept {
        for (size_t i = 0; i < std::decay_t<T>::COUNT; ++i)
            shape[i] = strides[i] ? shape[i] : 1;
        return shape;
    }

    /// Returns the order the dimensions should be sorted so that they are in the rightmost order.
    /// The input dimensions have the following indexes: {0, 1, 2, 3}.
    /// For instance, if \p strides is representing a F-contiguous order, this function returns {0, 1, 3, 2}.
    /// Empty dimensions are pushed to the left side (the outermost side) and their strides are ignored.
    /// This is mostly intended to find the fastest way through an array using nested loops in the rightmost order.
    /// \see indexing::reorder(...).
    template<typename T, typename = std::enable_if_t<traits::is_intX_v<T>>>
    NOA_IHD auto order(T strides, T shape) {
        using vec_t = traits::remove_ref_cv_t<T>;
        using int_t = traits::value_type_t<T>;
        constexpr size_t COUNT = vec_t::COUNT;

        vec_t order;
        for (size_t i = 0; i < COUNT; ++i) {
            order[i] = static_cast<int_t>(i);
            strides[i] = shape[i] <= 1 ? math::Limits<int_t>::max() : strides[i];
        }

        return math::sort(order, [&](int_t a, int_t b) {
            return strides[a] > strides[b];
        });
    }

    /// Returns the order the dimensions should be sorted so that empty dimensions are on the left.
    /// The input dimensions have the following indexes: {0, 1, 2, 3}.
    /// Coupled with indexing::reorder(), this effectively pushes all zeros and ones in \p shape to the left.
    /// The difference with indexing::order() is that this function does not change the order of the non-empty
    /// dimensions relative to each other. Note that the order of the empty dimensions is preserved.
    template<typename T, typename = std::enable_if_t<traits::is_intX_v<T>>>
    NOA_HD auto squeeze(T shape) {
        using vec_t = std::decay_t<T>;
        constexpr auto COUNT = static_cast<int>(vec_t::COUNT);

        vec_t order{};
        int idx = COUNT - 1;
        for (int i = idx; i >= 0; --i) {
            if (shape[i] > 1)
                order[idx--] = i;
            else
                order[idx - i] = i;
        }
        // The empty dimensions are entered in descending order.
        // To put them back to the original ascending order, we can do:
        if (idx) {
            if (COUNT >= 2 && idx == 1)
                smallStableSort<2>(order.get());
            if (COUNT >= 3 && idx == 2)
                smallStableSort<3>(order.get());
            else if (COUNT >= 4 && idx == 3)
                smallStableSort<4>(order.get());
        }

        // This seems unnecessary because the dimensions are empty so there order shouldn't matter...
        return order;
    }

    /// Reorder (i.e. sort) \p v according to the indexes in \p order.
    template<typename T, typename U, typename = std::enable_if_t<traits::is_int4_v<T> || traits::is_float4_v<T>>>
    NOA_FHD T reorder(T v, Int4<U> order) {
        return {v[order[0]], v[order[1]], v[order[2]], v[order[3]]};
    }
    template<typename T, typename U, typename = std::enable_if_t<traits::is_int3_v<T> || traits::is_float3_v<T>>>
    NOA_FHD T reorder(T v, Int3<U> order) {
        return {v[order[0]], v[order[1]], v[order[2]]};
    }
    template<typename T, typename U, typename = std::enable_if_t<traits::is_int2_v<T> || traits::is_float2_v<T>>>
    NOA_FHD T reorder(T v, Int2<U> order) {
        return {v[order[0]], v[order[1]]};
    }

    /// (Circular) shifts \p v by a given amount.
    /// If \p shift is positive, shifts to the right, otherwise, shifts to the left.
    template<typename T>
    NOA_FHD Int4<T> shift(Int4<T> v, int shift) {
        Int4 <T> out;
        const bool right = shift >= 0;
        if (shift < 0)
            shift *= -1;
        for (int i = 0; i < 4; ++i) {
            const int idx = (i + shift) % 4;
            out[idx * right + (1 - right) * i] = v[i * right + (1 - right) * idx];
        }
        return out;
    }

    /// Whether \p strides describes a column-major layout.
    /// Note that this functions assumes BDHW order, where H is the number of rows and W is the number of columns.
    template<typename T, typename = std::enable_if_t<traits::is_intX_v<T>>>
    NOA_FHD bool isColMajor(T&& strides) {
        constexpr size_t N = std::decay_t<T>::COUNT;
        constexpr size_t COL = N - 2;
        constexpr size_t ROW = N - 1;
        return strides[COL] <= strides[ROW];
    }

    /// Whether \p strides describes a column-major layout.
    /// This function effectively squeezes the shape before checking the order. Furthermore, strides of empty
    /// dimensions are ignored and are contiguous by definition.
    template<typename T, typename = std::enable_if_t<traits::is_intX_v<T>>>
    NOA_FHD bool isColMajor(T&& strides, T&& shape) {
        int second{-1}, first{-1};
        for (int i = std::decay_t<T>::COUNT - 1; i >= 0; --i) {
            if (shape[i] > 1) {
                if (first == -1)
                    first = i;
                else if (second == -1)
                    second = i;
            }
        }
        return second == -1 || first == -1 || strides[second] <= strides[first];
    }

    /// Whether \p strides describes a row-major layout.
    /// Note that this functions assumes BDHW order, where H is the number of rows and W is the number of columns.
    template<typename T, typename = std::enable_if_t<traits::is_intX_v<T>>>
    NOA_FHD bool isRowMajor(T&& strides) {
        constexpr size_t N = std::decay_t<T>::COUNT;
        constexpr size_t COL = N - 2;
        constexpr size_t ROW = N - 1;
        return strides[COL] >= strides[ROW];
    }

    /// Whether \p strides describes a row-major layout.
    /// This function effectively squeezes the shape before checking the order. Furthermore, strides of empty
    /// dimensions are ignored and are contiguous by definition.
    template<typename T, typename = std::enable_if_t<traits::is_intX_v<T>>>
    NOA_FHD bool isRowMajor(T&& strides, T&& shape) {
        int second{-1}, first{-1};
        for (int i = std::decay_t<T>::COUNT - 1; i >= 0; --i) {
            if (shape[i] > 1) {
                if (first == -1)
                    first = i;
                else if (second == -1)
                    second = i;
            }
        }
        return second == -1 || first == -1 || strides[second] >= strides[first];
    }

    /// Sets the input strides so that the input can be iterated as if it as the same size as the output.
    /// \param input_size           Size of the input. Should correspond to \p output_size or be 1.
    /// \param[out] input_strides   Input strides. If broadcast, it is set to 0.
    /// \param output_size          Size of the output.
    /// \return Whether the input and output size are compatible.
    template<typename T>
    NOA_IH bool broadcast(T input_size, T& input_strides, T output_size) noexcept {
        if (input_size == 1 && output_size != 1)
            input_strides = 0; // broadcast this dimension
        else if (input_size != output_size)
            return false; // dimension sizes don't match
        return true;
    }

    /// Sets the input stride so that the input can be iterated as if it as the same shape as the output.
    /// \param input_shape          Shape of the input. Each dimension should correspond to \p output_shape or be 1.
    /// \param[out] input_strides   Input strides. Strides in dimensions that need to be broadcast are set to 0.
    /// \param output_shape         Shape of the output.
    /// \return Whether the input and output shape are compatible.
    template<typename T>
    NOA_IH bool broadcast(Int4<T> input_shape, Int4<T>& input_strides, Int4<T> output_shape) noexcept {
        for (size_t i = 0; i < 4; ++i) {
            if (input_shape[i] == 1 && output_shape[i] != 1)
                input_strides[i] = 0; // broadcast this dimension
            else if (input_shape[i] != output_shape[i])
                return false; // dimension sizes don't match
        }
        return true;
    }

    /// Computes the new strides of an array after reshaping.
    /// \param old_shape        Old shape. An empty shape (dimension of 0) returns false.
    /// \param old_strides      Old strides.
    /// \param new_shape        New shape.
    /// \param[out] new_strides New strides.
    /// \return Whether the input and output shape and strides are compatible.
    ///         If false, \p new_strides is left in an undefined state.
    template<typename T>
    NOA_IH bool reshape(Int4<T> old_shape, Int4<T> old_strides,
                        Int4<T> new_shape, Int4<T>& new_strides) noexcept {
        // see https://github.com/pytorch/pytorch/blob/master/aten/src/ATen/TensorUtils.cpp
        if (!math::prod(old_shape))
            return false;

        int view_d = 3;
        T chunk_base_strides = old_strides[3];
        T tensor_numel = 1;
        T view_numel = 1;
        for (int tensor_d = 3; tensor_d >= 0; --tensor_d) {
            tensor_numel *= old_shape[tensor_d];
            // if end of tensor size chunk, check view
            if ((tensor_d == 0) ||
                (old_shape[tensor_d - 1] != 1 && old_strides[tensor_d - 1] != tensor_numel * chunk_base_strides)) {
                while (view_d >= 0 && (view_numel < tensor_numel || new_shape[view_d] == 1)) {
                    new_strides[view_d] = view_numel * chunk_base_strides;
                    view_numel *= new_shape[view_d];
                    --view_d;
                }

                if (view_numel != tensor_numel)
                    return false;
                if (tensor_d > 0) {
                    chunk_base_strides = old_strides[tensor_d - 1];
                    tensor_numel = 1;
                    view_numel = 1;
                }
            }
        }
        return view_d == -1;
    }

    /// Whether the range [lhs_start, lhs_end] overlaps with the range [rhs_start, rhs_end].
    constexpr inline bool isOverlap(std::uintptr_t lhs_start, std::uintptr_t lhs_end,
                                    std::uintptr_t rhs_start, std::uintptr_t rhs_end) noexcept {
        return lhs_start <= rhs_end && lhs_end >= rhs_start;
    }
}

namespace noa::indexing {
    /// Returns the 2D rightmost indexes corresponding to the given memory offset,
    /// assuming the innermost dimension is contiguous.
    /// \param offset   Linear memory offset.
    /// \param stride   Stride of the second-most dimension (i.e. pitch).
    /// \warning Broadcasting is not supported, so the stride should be greater than 0.
    template<typename T>
    NOA_FHD constexpr Int2<T> indexes(T offset, T stride) noexcept {
        NOA_ASSERT(stride > 0);
        const T i0 = offset / stride;
        const T i1 = offset - i0 * stride;
        return {i0, i1};
    }

    /// Returns the 3D rightmost indexes corresponding to the given memory offset.
    /// \param offset   Linear memory offset.
    /// \param p0,p1    Pitch of the second-most and innermost dimension.
    template<typename T>
    NOA_FHD constexpr Int3<T> indexes(T offset, T p0, T p1) noexcept {
        const T i0 = offset / (p0 * p1);
        offset -= i0 * p0 * p1;
        const T i1 = offset / p1;
        offset -= i1 * p1;
        return {i0, i1, offset};
    }

    /// Returns the 4D rightmost indexes corresponding to the given memory offset.
    /// \param offset   Linear memory offset.
    /// \param p0,p1,p2 Pitch of the third-most, second-most and innermost dimension.
    template<typename T>
    NOA_FHD constexpr Int4<T> indexes(T offset, T p0, T p1, T p2) noexcept {
        const T i0 = offset / (p0 * p1 * p2);
        offset -= i0 * p0 * p1 * p2;
        const T i1 = offset / (p1 * p2);
        offset -= i1 * p1 * p2;
        const T i2 = offset / p2;
        offset -= i2 * p2;
        return {i0, i1, i2, offset};
    }

    /// Returns the 4D rightmost indexes corresponding to the given memory offset.
    /// \param offset   Linear memory offset.
    /// \param pitch    Pitch of the third-most, second-most and innermost dimension.
    template<typename T>
    NOA_FHD constexpr Int4<T> indexes(T offset, Int3<T> pitch) noexcept {
        return {offset, pitch[0], pitch[1], pitch[2]};
    }

    /// Returns the multidimensional indexes corresponding to a memory \p offset.
    /// \details Given a memory offset and a memory layout (i.e. strides and shape), this function computes
    ///          the 4D logical indexes. Broadcasting is not supported, so the strides should be greater than 0.
    ///          Otherwise, any ordering is supported.
    /// \param offset   Memory offset with the array.
    /// \param strides  Strides of the array.
    /// \param shape    Shape of the array.
    template<typename T, typename U, typename = std::enable_if_t<traits::is_intX_v<U>>>
    NOA_IHD constexpr auto indexes(T offset, U strides, U shape) noexcept {
        NOA_ASSERT(all(shape > 0));
        using vec_t = traits::remove_ref_cv_t<U>;
        using val_t = traits::value_type_t<U>;

        const vec_t rightmost_order = indexing::order(strides, shape);

        vec_t out;
        T remain = offset;
        for (size_t i = 0; i < vec_t::COUNT; ++i) {
            const val_t idx = rightmost_order[i];
            if (shape[idx] > 1) { // if empty, ignore it.
                NOA_ASSERT(strides[idx] > 0);
                out[idx] = remain / strides[idx]; // single-divide optimization should kick in
                remain %= strides[idx]; // or remain -= out[i] * stride
            }
        }
        NOA_ASSERT(remain == 0);
        return out;
    }
}

namespace noa::indexing {
    /// Reinterpret (cast) a 4D array.
    /// \details
    ///        1. Create an object with the original shape, strides and pointer of the array to reinterpret.\n
    ///        2. Call the as<V> method to reinterpret the T array as a V array. If sizeof(T) == sizeof(V), then this
    ///           is equivalent to calling reinterpret_cast<V*> on the original T pointer.\n
    ///        3. Get the new shape, stride, and pointer from the output of the as<V> method.\n
    /// \note Reinterpretation is not always possible/well-defined. T and V types, as well as the original shape/strides
    ///       should be compatible, otherwise an error will be thrown. This is mostly to represent any data type as a
    ///       array of bytes, or to switch between complex and real floating-point numbers with the same precision.
    template<typename T, typename I = size_t>
    struct Reinterpret {
    public:
        static_assert(std::is_integral_v<I>);
        Int4<I> shape{};
        Int4<I> strides{};
        T* ptr{};

    public:
        template<typename U, typename = std::enable_if_t<std::is_integral_v<U>>>
        constexpr Reinterpret(Int4<U> a_shape, Int4<U> a_strides, T* a_ptr) noexcept
                : shape{a_shape}, strides{a_strides}, ptr{a_ptr} {}

        template<typename V>
        Reinterpret<V> as() const {
            using origin_t = T;
            using new_t = V;
            Reinterpret<V> out{shape, strides, reinterpret_cast<V*>(ptr)};

            // The "downsize" and "upsize" branches expects the strides and shape to be in the rightmost order.
            const size4_t rightmost_order = order(out.strides, out.shape);

            if constexpr (sizeof(origin_t) > sizeof(new_t)) { // downsize
                constexpr I ratio = sizeof(origin_t) / sizeof(new_t);
                NOA_CHECK(strides[rightmost_order[3]] == 1,
                          "The stride of the innermost dimension must be 1 to view a {} as a {}",
                          string::human<origin_t>(), string::human<new_t>());
                out.strides[rightmost_order[0]] *= ratio;
                out.strides[rightmost_order[1]] *= ratio;
                out.strides[rightmost_order[2]] *= ratio;
                out.strides[rightmost_order[3]] = 1;
                out.shape[rightmost_order[3]] *= ratio;

            } else if constexpr (sizeof(origin_t) < sizeof(new_t)) { // upsize
                constexpr I ratio = sizeof(new_t) / sizeof(origin_t);
                static_assert(alignof(cdouble_t) == 16);
                NOA_CHECK(out.shape[rightmost_order[3]] % ratio == 0,
                          "The size of the innermost dimension must be divisible by {} to view a {} as a {}",
                          ratio, string::human<origin_t>(), string::human<new_t>());
                NOA_CHECK(!(reinterpret_cast<std::uintptr_t>(ptr) % alignof(new_t)),
                          "The memory offset should at least be aligned to {} bytes to be viewed as a {}, but got {}",
                          alignof(new_t), string::human<new_t>(), static_cast<const void*>(ptr));
                NOA_CHECK(out.strides[rightmost_order[3]] == 1,
                          "The stride of the innermost dimension must be 1 to view a {} as a {}",
                          string::human<origin_t>(), string::human<new_t>());
                for (int i = 0; i < 3; ++i) {
                    NOA_CHECK(!(out.strides[i] % ratio), "The strides must be divisible by {} to view a {} as a {}",
                              ratio, string::human<origin_t>(), string::human<new_t>());
                    out.strides[i] /= ratio;
                }
                out.strides[rightmost_order[3]] = 1;
                out.shape[rightmost_order[3]] /= ratio;
            }
            return out;
        }
    };
}

namespace noa::indexing {
    /// Ellipsis or "..." operator, which selects the full extent of the remaining outermost dimension(s).
    struct ellipsis_t {};

    /// Selects the entire the dimension.
    struct full_extent_t {};

    /// Slice operator. The start and end can be negative and out of bound. The step must be non-zero positive.
    struct slice_t {
        template<typename T = int64_t, typename U = int64_t, typename V = int64_t>
        constexpr explicit slice_t(T start_ = 0, U end_ = std::numeric_limits<int64_t>::max(), V step_ = V{1})
                : start(static_cast<int64_t>(start_)),
                  end(static_cast<int64_t>(end_)),
                  step(static_cast<int64_t>(step_)) {}

        int64_t start{};
        int64_t end{};
        int64_t step{};
    };

    /// Splits a [0, \p size) range into \p n output \p slices of approximately equal length.
    NOA_IH void split(size_t size, size_t n, slice_t* slices) {
        const int count = static_cast<int>(n);
        const int size_ = static_cast<int>(size);
        for (int i = 0; i < count; ++i) {
            const int k = size_ / count;
            const int m = size_ % count;
            const int slice_start = i * k + noa::math::min(i, m);
            const int slice_end = (i + 1) * k + noa::math::min(i + 1, m);
            slices[i] = slice_t{slice_start, slice_end};
        }
    }

    /// Utility for indexing subregions.
    /// The indexing operator is bound-checked.
    struct Subregion {
    private:
        template<typename U>
        static constexpr bool is_indexer_v =
                std::bool_constant<noa::traits::is_int_v<U> ||
                                   noa::traits::is_almost_same_v<U, indexing::full_extent_t> ||
                                   noa::traits::is_almost_same_v<U, indexing::slice_t>>::value;

    public:
        constexpr Subregion() = default;

        template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        constexpr Subregion(Int4<T> a_shape, Int4<T> a_strides, T a_offset = T{0}) noexcept
                : m_shape{a_shape}, m_strides{a_strides}, m_offset{static_cast<size_t>(a_offset)} {}

        template<typename A,
                 typename B = indexing::full_extent_t,
                 typename C = indexing::full_extent_t,
                 typename D = indexing::full_extent_t,
                 typename = std::enable_if_t<is_indexer_v<A> && is_indexer_v<B> &&
                                             is_indexer_v<C> && is_indexer_v<D>>>
        constexpr Subregion operator()(A&& i0, B&& i1 = {}, C&& i2 = {}, D&& i3 = {}) const {
            Subregion out{};
            indexDim_(i0, 0, m_shape[0], m_strides[0], out.m_shape.get() + 0, out.m_strides.get() + 0, &out.m_offset);
            indexDim_(i1, 1, m_shape[1], m_strides[1], out.m_shape.get() + 1, out.m_strides.get() + 1, &out.m_offset);
            indexDim_(i2, 2, m_shape[2], m_strides[2], out.m_shape.get() + 2, out.m_strides.get() + 2, &out.m_offset);
            indexDim_(i3, 3, m_shape[3], m_strides[3], out.m_shape.get() + 3, out.m_strides.get() + 3, &out.m_offset);
            return out;
        }

        constexpr Subregion operator()(indexing::ellipsis_t) const {
            return *this;
        }

        template<typename A, typename = std::enable_if_t<is_indexer_v<A>>>
        constexpr Subregion operator()(indexing::ellipsis_t, A&& i3) const {
            return (*this)(indexing::full_extent_t{}, indexing::full_extent_t{}, indexing::full_extent_t{}, i3);
        }

        template<typename A, typename B,
                 typename = std::enable_if_t<is_indexer_v<A> && is_indexer_v<B>>>
        constexpr Subregion operator()(indexing::ellipsis_t, A&& i2, B&& i3) const {
            return (*this)(indexing::full_extent_t{}, indexing::full_extent_t{}, i2, i3);
        }

        template<typename A, typename B, typename C,
                 typename = std::enable_if_t<is_indexer_v<A> && is_indexer_v<B> && is_indexer_v<C>>>
        constexpr Subregion operator()(indexing::ellipsis_t, A&& i1, B&& i2, C&& i3) const {
            return (*this)(indexing::full_extent_t{}, i1, i2, i3);
        }

    public:
        [[nodiscard]] constexpr size4_t shape() const noexcept { return size4_t{m_shape}; }
        [[nodiscard]] constexpr size4_t strides() const noexcept { return m_strides; }
        [[nodiscard]] constexpr size_t offset() const noexcept { return m_offset; }

    private:
        // Compute the new size, strides and offset, for one dimension, given an indexing mode (integral, slice or full).
        template<typename IndexMode>
        static constexpr void indexDim_(IndexMode idx_mode, int64_t dim,
                                        int64_t old_size, size_t old_strides,
                                        int64_t* new_size, size_t* new_strides,
                                        size_t* new_offset) {
            if constexpr (traits::is_int_v<IndexMode>) {
                auto index = clamp_cast<int64_t>(idx_mode);
                NOA_CHECK(!(index < -old_size || index >= old_size),
                          "Index {} is out of range for a size of {} at dimension {}", index, old_size, dim);

                if (index < 0)
                    index += old_size;
                *new_strides = old_strides; // or 0
                *new_size = 1;
                *new_offset += old_strides * static_cast<size_t>(index);

            } else if constexpr(std::is_same_v<indexing::full_extent_t, IndexMode>) {
                *new_strides = old_strides;
                *new_size = old_size;
                *new_offset += 0;
                (void) idx_mode;
                (void) dim;

            } else if constexpr(std::is_same_v<indexing::slice_t, IndexMode>) {
                NOA_CHECK(idx_mode.step > 0, "Slice step must be positive, got {}", idx_mode.step);

                if (idx_mode.start < 0)
                    idx_mode.start += old_size;
                if (idx_mode.end < 0)
                    idx_mode.end += old_size;

                idx_mode.start = noa::math::clamp(idx_mode.start, int64_t{0}, old_size);
                idx_mode.end = noa::math::clamp(idx_mode.end, idx_mode.start, old_size);

                *new_size = noa::math::divideUp(idx_mode.end - idx_mode.start, idx_mode.step);
                *new_strides = old_strides * static_cast<size_t>(idx_mode.step);
                *new_offset += static_cast<size_t>(idx_mode.start) * old_strides;
                (void) dim;
            } else {
                static_assert(traits::always_false_v<IndexMode>);
            }
        }

    private:
        long4_t m_shape{};
        size4_t m_strides{};
        size_t m_offset{};
    };
}
