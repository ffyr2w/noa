#pragma once

#include "noa/common/Definitions.h"
#include "noa/common/Types.h"
#include "noa/cpu/Stream.h"
#include "noa/common/Functors.h"

namespace noa::cpu::math::details {
    template<typename S, typename T, typename U>
    constexpr bool is_valid_find_v =
            traits::is_any_v<T, uint32_t, uint64_t, int32_t, int64_t, half_t, float, double> &&
            traits::is_any_v<U, uint32_t, uint64_t, int32_t, int64_t> &&
            traits::is_any_v<S, noa::math::first_min_t, noa::math::first_max_t, noa::math::last_min_t, noa::math::last_max_t>;
}

namespace noa::cpu::math {
    // Returns the memory offset(s) of a particular kind of value(s).
    template<typename S, typename T, typename U, typename = std::enable_if_t<details::is_valid_find_v<S, T, U>>>
    void find(S searcher, const shared_t<T[]>& input, size4_t strides, size4_t shape,
              const shared_t<U[]>& offsets, bool batch, bool swap_layout, Stream& stream);

    // Returns the memory offset of a particular kind of value.
    template<typename offset_t = size_t, typename S, typename T,
             typename = std::enable_if_t<details::is_valid_find_v<S, T, offset_t>>>
    offset_t find(S searcher, const shared_t<T[]>& input, size4_t strides, size4_t shape,
                  bool swap_layout, Stream& stream);

    // Returns the index of a particular kind of value.
    template<typename offset_t = size_t, typename S, typename T,
             typename = std::enable_if_t<details::is_valid_find_v<S, T, offset_t>>>
    offset_t find(S searcher, const shared_t<T[]>& input, size_t elements, Stream& stream);
}
