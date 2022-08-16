#pragma once

#include "noa/common/Definitions.h"
#include "noa/gpu/cuda/Types.h"
#include "noa/gpu/cuda/Stream.h"

namespace noa::cuda::memory::details {
    template<typename T, typename U>
    constexpr bool is_valid_cast_v =
            (traits::is_any_v<T, bool, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, half_t, float, double> &&
             traits::is_any_v<U, bool, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, half_t, float, double>) ||
            (traits::is_complex_v<T> && traits::is_complex_v<U>);
}

namespace noa::cuda::memory {
    // Casts one array to another type.
    template<typename T, typename U, typename = std::enable_if_t<details::is_valid_cast_v<T, U>>>
    void cast(const shared_t<T[]>& input,
              const shared_t<U[]>& output,
              size_t elements, bool clamp, Stream& stream);

    // Casts one array to another type.
    template<typename T, typename U, typename = std::enable_if_t<details::is_valid_cast_v<T, U>>>
    void cast(const shared_t<T[]>& input, size4_t input_strides,
              const shared_t<U[]>& output, size4_t output_strides,
              size4_t shape, bool clamp, Stream& stream);
}
