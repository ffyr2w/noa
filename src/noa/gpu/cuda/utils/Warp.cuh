#pragma once

#include <cuda_fp16.h>

#include "noa/core/Definitions.hpp"
#include "noa/gpu/cuda/Types.hpp"

namespace noa::cuda::utils::details {
    template<typename T>
    constexpr bool is_valid_suffle_v = noa::traits::is_numeric_v<T> || noa::traits::is_any_v<T, half, half2>;
}

namespace noa::cuda::utils {
    template <typename T, typename = std::enable_if_t<details::is_valid_suffle_v<T>>>
    NOA_FD T warp_shuffle(T value, int32_t source, int32_t width = 32, uint32_t mask = 0xffffffff) {
        if constexpr (noa::traits::is_almost_same_v<c16, T>) {
            __half2 tmp = __shfl_sync(mask, *reinterpret_cast<__half2*>(&value), source, width);
            return *reinterpret_cast<c16*>(&tmp);
        } else if constexpr (std::is_same_v<f16, T>) {
            return T(__shfl_sync(mask, value.native(), source, width));
        } else if constexpr (noa::traits::is_complex_v<T>) {
            return T(__shfl_sync(mask, value.real, source, width),
                     __shfl_sync(mask, value.imag, source, width));
        } else if (noa::traits::is_int_v<T> && sizeof(T) < 4) {
            return static_cast<T>(__shfl_sync(mask, static_cast<int32_t>(value), source, width));
        } else {
            return __shfl_sync(mask, value, source, width);
        }
        return T{}; // unreachable
    }

    template <typename T, typename = std::enable_if_t<details::is_valid_suffle_v<T>>>
    NOA_FD T warp_suffle_down(T value, uint32_t delta, int32_t width = 32, uint32_t mask = 0xffffffff) {
        if constexpr (noa::traits::is_almost_same_v<c16, T>) {
            __half2 tmp = __shfl_down_sync(mask, *reinterpret_cast<__half2*>(&value), delta, width);
            return *reinterpret_cast<c16*>(&tmp);
        } else if constexpr (std::is_same_v<f16, T>) {
            return T(__shfl_down_sync(mask, value.native(), delta, width));
        } else if constexpr (noa::traits::is_complex_v<T>) {
            return T(__shfl_down_sync(mask, value.real, delta, width),
                     __shfl_down_sync(mask, value.imag, delta, width));
        } else if (noa::traits::is_int_v<T> && sizeof(T) < 4) {
            return static_cast<T>(__shfl_down_sync(mask, static_cast<int32_t>(value), delta, width));
        } else {
            return __shfl_down_sync(mask, value, delta, width);
        }
        return T{}; // unreachable
    }

    // Reduces one warp to one element.
    // The first thread of the warp returns with the reduced value.
    // The returned value is undefined for the other threads.
    // value:       Per-thread value.
    // reduce_op:   Reduction operator.
    template<typename Value, typename ReduceOp,
             typename = std::enable_if_t<noa::traits::is_numeric_v<Value>>>
    NOA_ID Value warp_reduce(Value value, ReduceOp reduce_op) {
        Value reduce;
        for (i32 delta = 1; delta < 32; delta *= 2) {
            reduce = warp_suffle_down(value, delta);
            value = reduce_op(reduce, value);
        }
        return value;
    }

    // Overload for pairs.
    template<typename Lhs, typename Rhs, typename ReduceOp>
    NOA_ID auto warp_reduce(Pair<Lhs, Rhs> pair, ReduceOp reduce_op) {
        using pair_t = Pair<Lhs, Rhs>;
        for (i32 delta = 1; delta < 32; delta *= 2) {
            pair_t reduce{warp_suffle_down(pair.first, delta),
                          warp_suffle_down(pair.second, delta)};
            pair = reduce_op(reduce, pair);
        }
        return pair;
    }
}
