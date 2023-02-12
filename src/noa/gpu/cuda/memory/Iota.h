#pragma once

#include "noa/core/Definitions.hpp"
#include "noa/core/Types.hpp"
#include "noa/gpu/cuda/Stream.h"

namespace noa::cuda::memory {
    // Returns a tiled sequence [0, elements).
    template<typename T, typename = std::enable_if_t<traits::is_restricted_scalar_v<T>>>
    void iota(const Shared<T[]>& src, i64 elements, i64 tile, Stream& stream);

    // Returns a tiled sequence [0, elements), in the rightmost order.
    template<typename T, typename = std::enable_if_t<traits::is_restricted_scalar_v<T>>>
    void iota(const Shared<T[]>& src, const Strides4<i64>& strides, const Shape4<i64>& shape,
              const Vec4<i64>& tile, Stream& stream);
}
