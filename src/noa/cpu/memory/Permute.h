#pragma once

#include "noa/common/Definitions.h"
#include "noa/common/Types.h"
#include "noa/cpu/Stream.h"

namespace noa::cpu::memory {
    // Permutes, in memory, the axes of an array.
    template<typename T, typename = std::enable_if_t<traits::is_restricted_data_v<T>>>
    void permute(const shared_t<T[]>& input, size4_t input_strides, size4_t input_shape,
                 const shared_t<T[]>& output, size4_t output_strides, uint4_t permutation, Stream& stream);
}

#define NOA_TRANSPOSE_INL_
#include "noa/cpu/memory/Permute.inl"
#undef NOA_TRANSPOSE_INL_
