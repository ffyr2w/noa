#pragma once

#include "noa/common/Definitions.h"
#include "noa/common/Types.h"
#include "noa/cpu/Stream.h"

namespace noa::cpu::memory {
    // Casts one array to another type.
    template<typename T, typename U>
    void cast(const shared_t<T[]>& input, const shared_t<U[]>& output,
              dim_t elements, bool clamp, Stream& stream) {
        NOA_ASSERT(input && output);
        stream.enqueue([=]() {
            const T* input_ = input.get();
            U* output_ = output.get();
            for (dim_t i = 0; i < elements; ++i, ++input_, ++output_)
                *output_ = clamp ? clamp_cast<U>(*input_) : static_cast<U>(*input_);
        });
    }

    // Casts one array to another type.
    template<bool SWAP_LAYOUT = true, typename T, typename U>
    void cast(const shared_t<T[]>& input, dim4_t input_strides,
              const shared_t<U[]>& output, dim4_t output_strides,
              dim4_t shape, bool clamp, Stream& stream) {
        if constexpr (SWAP_LAYOUT) {
            const dim4_t order = indexing::order(output_strides, shape);
            shape = indexing::reorder(shape, order);
            output_strides = indexing::reorder(output_strides, order);
            input_strides = indexing::reorder(input_strides, order);
        }

        if (indexing::areContiguous(input_strides, shape) &&
            indexing::areContiguous(output_strides, shape))
            return cast(input, output, shape.elements(), clamp, stream);

        NOA_ASSERT(input && output && all(shape > 0));
        stream.enqueue([=]() {
            const T* input_ = input.get();
            U* output_ = output.get();
            for (dim_t i = 0; i < shape[0]; ++i) {
                for (dim_t j = 0; j < shape[1]; ++j) {
                    for (dim_t k = 0; k < shape[2]; ++k) {
                        for (dim_t l = 0; l < shape[3]; ++l) {
                            const T value = input_[indexing::at(i, j, k, l, input_strides)];
                            output_[indexing::at(i, j, k, l, output_strides)] =
                                    clamp ? clamp_cast<U>(value) : static_cast<U>(value);
                        }
                    }
                }
            }
        });
    }
}
