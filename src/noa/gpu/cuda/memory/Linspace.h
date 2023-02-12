#pragma once

#include "noa/core/Definitions.hpp"
#include "noa/gpu/cuda/Types.h"
#include "noa/gpu/cuda/Stream.h"

namespace noa::cuda::memory {
    // Returns evenly spaced values within a given interval.
    template<typename T, typename = std::enable_if_t<traits::is_restricted_numeric_v<T>>>
    T linspace(const Shared<T[]>& src, i64 elements,
               T start, T stop, bool endpoint, Stream& stream);

    // Returns evenly spaced values within a given interval, in the rightmost order.
    template<typename T, typename = std::enable_if_t<traits::is_restricted_numeric_v<T>>>
    T linspace(const Shared<T[]>& src, const Strides4<i64>& strides, const Shape4<i64>& shape,
               T start, T stop, bool endpoint, Stream& stream);
}
