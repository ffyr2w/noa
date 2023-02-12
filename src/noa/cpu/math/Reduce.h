#pragma once

#include <algorithm>

#include "noa/core/Definitions.hpp"
#include "noa/core/Types.hpp"
#include "noa/core/Math.hpp"
#include "noa/cpu/Stream.hpp"

namespace noa::cpu::math::details {
    template<typename T>
    constexpr bool is_valid_min_max_median_v =
            noa::traits::is_any_v<T, i16, i32, i64, u16, u32, u64, f16, f32, f64>;

    template<typename T>
    constexpr bool is_valid_sum_mean_v =
            traits::is_any_v<T, i32, i64, u32, u64, f32, f64, c32, c64>;

    template<typename T, typename U>
    constexpr bool is_valid_var_std_v =
            traits::is_any_v<T, f32, f64, c32, c64> &&
            std::is_same_v<U, traits::value_type_t<T>>;
}

namespace noa::cpu::math {
    template<typename Value, typename = std::enable_if_t<details::is_valid_min_max_median_v<Value>>>
    [[nodiscard]] Value min(const Shared<Value[]>& input,
                            const Strides4<i64>& strides,
                            const Shape4<i64>& shape,
                            Stream& stream);

    template<typename Value, typename = std::enable_if_t<details::is_valid_min_max_median_v<Value>>>
    [[nodiscard]] Value max(const Shared<Value[]>& input,
                            const Strides4<i64>& strides,
                            const Shape4<i64>& shape,
                            Stream& stream);

    template<typename Value, typename = std::enable_if_t<details::is_valid_sum_mean_v<Value>>>
    [[nodiscard]] Value sum(const Shared<Value[]>& input,
                            const Strides4<i64>& strides,
                            const Shape4<i64>& shape,
                            Stream& stream);

    template<typename Value, typename = std::enable_if_t<details::is_valid_sum_mean_v<Value>>>
    [[nodiscard]] Value mean(const Shared<Value[]>& input,
                            const Strides4<i64>& strides,
                            const Shape4<i64>& shape,
                            Stream& stream);

    template<typename Input, typename Output = noa::traits::value_type_t<Input>,
             typename = std::enable_if_t<details::is_valid_var_std_v<Input, Output>>>
    [[nodiscard]] Output var(const Shared<Input[]>& input,
                             const Strides4<i64>& strides,
                             const Shape4<i64>& shape,
                             i64 ddof, Stream& stream);

    template<typename Input, typename Output = noa::traits::value_type_t<Input>,
             typename = std::enable_if_t<details::is_valid_var_std_v<Input, Output>>>
    [[nodiscard]] auto mean_var(const Shared<Input[]>& input,
                                const Strides4<i64>& strides,
                                const Shape4<i64>& shape,
                                i64 ddof, Stream& stream) -> std::pair<Input, Output>;

    template<typename Input, typename Output = noa::traits::value_type_t<Input>,
             typename = std::enable_if_t<details::is_valid_var_std_v<Input, Output>>>
    [[nodiscard]] Output std(const Shared<Input[]>& input,
                             const Strides4<i64>& strides,
                             const Shape4<i64>& shape,
                             i64 ddof, Stream& stream);

    template<typename Value, typename = std::enable_if_t<details::is_valid_min_max_median_v<Value>>>
    [[nodiscard]] Value median(const Shared<Value[]>& input,
                               Strides4<i64> strides,
                               Shape4<i64> shape,
                               bool overwrite,
                               Stream& stream);
}

namespace noa::cpu::math {
    template<typename Value, typename = std::enable_if_t<details::is_valid_min_max_median_v<Value>>>
    void min(const Shared<Value[]>& input, const Strides4<i64>& input_strides, const Shape4<i64>& input_shape,
             const Shared<Value[]>& output, const Strides4<i64>& output_strides, const Shape4<i64>& output_shape,
             Stream& stream);

    template<typename Value, typename = std::enable_if_t<details::is_valid_min_max_median_v<Value>>>
    void max(const Shared<Value[]>& input, const Strides4<i64>& input_strides, const Shape4<i64>& input_shape,
             const Shared<Value[]>& output, const Strides4<i64>& output_strides, const Shape4<i64>& output_shape,
             Stream& stream);

    template<typename Value, typename = std::enable_if_t<details::is_valid_sum_mean_v<Value>>>
    void sum(const Shared<Value[]>& input, const Strides4<i64>& input_strides, const Shape4<i64>& input_shape,
             const Shared<Value[]>& output, const Strides4<i64>& output_strides, const Shape4<i64>& output_shape,
             Stream& stream);

    template<typename Value, typename = std::enable_if_t<details::is_valid_sum_mean_v<Value>>>
    void mean(const Shared<Value[]>& input, const Strides4<i64>& input_strides, const Shape4<i64>& input_shape,
              const Shared<Value[]>& output, const Strides4<i64>& output_strides, const Shape4<i64>& output_shape,
              Stream& stream);

    template<typename Input, typename Output, typename = std::enable_if_t<details::is_valid_var_std_v<Input, Output>>>
    void var(const Shared<Input[]>& input, const Strides4<i64>& input_strides, const Shape4<i64>& input_shape,
             const Shared<Output[]>& output, const Strides4<i64>& output_strides, const Shape4<i64>& output_shape,
             i64 ddof, Stream& stream);

    template<typename Input, typename Output, typename = std::enable_if_t<details::is_valid_var_std_v<Input, Output>>>
    void std(const Shared<Input[]>& input, const Strides4<i64>& input_strides, const Shape4<i64>& input_shape,
             const Shared<Output[]>& output, const Strides4<i64>& output_strides, const Shape4<i64>& output_shape,
             i64 ddof, Stream& stream);
}
