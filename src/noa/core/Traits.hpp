#pragma once

#include "noa/core/Config.hpp"
#include "noa/core/Namespace.hpp"

#if defined(NOA_IS_OFFLINE)
#include <climits>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#else
#include <cuda/std/climits>
#include <cuda/std/cstddef>
#include <cuda/std/cstdint>
#include <cuda/std/type_traits>
#include <cuda/std/utility>
#endif

// Assume POSIX and/or Windows, both of which guarantee CHAR_BIT == 8.
// The rest should fine for all modern hardware.
static_assert(CHAR_BIT == 8);
static_assert(sizeof(short) == 2);
static_assert(sizeof(int) == 4);
static_assert(sizeof(float) == 4);
static_assert(std::is_same_v<int8_t, signed char>);
static_assert(std::is_same_v<uint8_t, unsigned char>);
static_assert(std::is_same_v<int16_t, signed short>);
static_assert(std::is_same_v<uint16_t, unsigned short>);
static_assert(std::is_same_v<int32_t, signed int>);
static_assert(std::is_same_v<uint32_t, unsigned int>);

namespace noa::inline types {
    struct Empty {};
    using Byte = std::byte;

    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;

    using f32 = float;
    using f64 = double;
    static_assert(sizeof(f32) == 4);
    static_assert(sizeof(f64) == 8);
}

namespace noa::traits {
    template<typename T> struct remove_ref_cv { using type = typename std::remove_cv_t<typename std::remove_reference_t<T>>; };
    template<typename T> using remove_ref_cv_t = typename remove_ref_cv<T>::type;

    template<typename T> struct remove_pointer_cv { using type = typename std::remove_cv_t<typename std::remove_pointer_t<T>>; };
    template<typename T> using remove_pointer_cv_t = typename remove_pointer_cv<T>::type;

    template <bool... Bs> using bool_sequence_t = std::integer_sequence<bool, Bs...>;
    template <bool... Bs> using bool_and = std::is_same<bool_sequence_t<Bs...>, bool_sequence_t<(Bs || true)...>>;
    template <bool... Bs> using bool_or = std::integral_constant<bool, !bool_and<!Bs...>::value>;

    template<typename T> using always_false = std::false_type;
    template<typename T> constexpr bool always_false_v = always_false<T>::value;

    template<bool B>
    using enable_if_bool_t = std::enable_if_t<B, bool>;

    template<typename T>
    using identity_t = T;

    template<typename First, typename...>
    using first_t = First;

    template<typename... T>
    struct TypeList {};

    template<typename... Ls, typename... Rs>
    inline constexpr auto operator+(TypeList<Ls...>, TypeList<Rs...>) {
        return TypeList<Ls..., Rs...>{};
    }

    namespace guts {
        template<typename, typename>
        struct append_to_type_seq {};

        template<typename T, typename... Ts, template<typename...> class TT>
        struct append_to_type_seq<T, TT<Ts...>> {
            using type = TT<Ts..., T>;
        };
    }

    template<typename T, size_t N, template<typename...> class TT>
    struct repeat {
        using type = guts::append_to_type_seq<T, typename repeat<T, N - 1, TT>::type>::type;
    };

    template<typename T, template<typename...> class TT>
    struct repeat<T, 0, TT> {
        using type = TT<>;
    };
}

namespace noa::traits {
    namespace guts {
        template<typename T, typename = void> struct type_type { using type = T; };
        template<typename T, typename = void> struct value_type { using type = T; };
        template<typename T, typename = void> struct mutable_value_type { using type = T; };
        template<typename T, typename = void> struct element_type { using type = T; };
        template<typename T, typename = void> struct index_type { using type = T; };
        template<typename T, typename = void> struct shared_type { using type = T; };
        template<typename T, typename = void> struct pointer_type { using type = T; };

        template<typename T> struct type_type<T, std::void_t<typename T::type_type>> { using type = typename T::type; };
        template<typename T> struct value_type<T, std::void_t<typename T::value_type>> { using type = typename T::value_type; };
        template<typename T> struct mutable_value_type<T, std::void_t<typename T::mutable_value_type>> { using type = typename T::mutable_value_type; };
        template<typename T> struct element_type<T, std::void_t<typename T::element_type>> { using type = typename T::element_type; };
        template<typename T> struct index_type<T, std::void_t<typename T::index_type>> { using type = typename T::index_type; };
        template<typename T> struct pointer_type<T, std::void_t<typename T::pointer_type>> { using type = typename T::pointer_type; };
        template<typename T> struct shared_type<T, std::void_t<typename T::shared_type>> { using type = typename T::shared_type; };
    }

    template<typename T> struct value_type { using type = typename guts::value_type<T>::type; };
    template<typename T> struct mutable_value_type { using type = typename guts::mutable_value_type<T>::type; };
    template<typename T> struct element_type { using type = typename guts::element_type<T>::type; };
    template<typename T> struct index_type { using type = typename guts::index_type<T>::type; };
    template<typename T> struct pointer_type { using type = typename guts::pointer_type<T>::type; };
    template<typename T> struct shared_type { using type = typename guts::shared_type<T>::type; };

    template<typename T> using type_t = typename guts::type_type<T>::type;
    template<typename T> using value_type_t = typename value_type<std::decay_t<T>>::type;
    template<typename T> using value_type_twice_t = value_type_t<value_type_t<T>>;
    template<typename T> using mutable_value_type_t = typename mutable_value_type<std::decay_t<T>>::type;
    template<typename T> using element_type_t = typename element_type<std::decay_t<T>>::type;
    template<typename T> using index_type_t = typename index_type<std::decay_t<T>>::type;
    template<typename T> using pointer_type_t = typename pointer_type<std::decay_t<T>>::type;
    template<typename T> using shared_type_t = typename shared_type<std::decay_t<T>>::type;

    template<typename InputValue, typename OutputValue>
    constexpr bool is_mutable_value_type_v =
            std::is_const_v<OutputValue> &&
            std::is_same_v<InputValue, std::remove_const_t<OutputValue>>;
}

namespace noa::traits {
    template<typename T1, typename T2> using is_almost_same = std::bool_constant<std::is_same_v<std::decay_t<T1>, std::decay_t<T2>>>;
    template<typename T1, typename T2> constexpr bool is_almost_same_v = is_almost_same<T1, T2>::value;

    template<typename T, typename... Ts> struct is_any : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};
    template<typename T, typename... Ts> constexpr bool is_any_v = is_any<T, Ts...>::value;

    template<typename T, typename... Ts> struct is_almost_any : std::bool_constant<(is_almost_same_v<T, Ts> || ...)> {};
    template<typename T, typename... Ts> constexpr bool is_almost_any_v = is_almost_any<T, Ts...>::value;

    template<typename T, typename... Ts> struct are_all_same : std::bool_constant<(std::is_same_v<T, Ts> && ...)> {};
    template<typename T, typename... Ts> constexpr bool are_all_same_v = are_all_same<T, Ts...>::value;

    template<typename T, typename... Ts> struct are_almost_all_same : std::bool_constant<(is_almost_same_v<T, Ts> && ...)> {};
    template<typename T, typename... Ts> constexpr bool are_almost_all_same_v = are_almost_all_same<T, Ts...>::value;

    template<typename T, typename... Ts> struct are_almost_same_value_type : std::bool_constant<(is_almost_same_v<value_type_t<T>, value_type_t<Ts>> && ...)> {};
    template<typename T, typename... Ts> constexpr bool are_almost_same_value_type_v = are_almost_same_value_type<T, Ts...>::value;

    template<typename T, typename... Ts> struct are_same_value_type : std::bool_constant<(std::is_same_v<value_type_t<T>, value_type_t<Ts>> && ...)> {};
    template<typename T, typename... Ts> constexpr bool are_same_value_type_v = are_same_value_type<T, Ts...>::value;
}

namespace noa::traits {
    // boolean
    template<typename> struct proclaim_is_bool : std::false_type {};
    template<> struct proclaim_is_bool<bool> : std::true_type {};
    template<typename T> using is_bool = std::bool_constant<proclaim_is_bool<T>::value>;
    template<typename T> constexpr bool is_bool_v = is_bool<std::decay_t<T>>::value;
    template<typename... Ts> using are_bool = bool_and<is_bool<Ts>::value...>;
    template<typename... Ts> constexpr bool are_bool_v = are_bool<std::decay_t<Ts>...>::value;

    // any unsigned integer
    template<typename> struct proclaim_is_uint : std::false_type {};
    template<> struct proclaim_is_uint<bool> : std::true_type {};
    template<> struct proclaim_is_uint<unsigned char> : std::true_type {};
    template<> struct proclaim_is_uint<unsigned short> : std::true_type {};
    template<> struct proclaim_is_uint<unsigned int> : std::true_type {};
    template<> struct proclaim_is_uint<unsigned long> : std::true_type {};
    template<> struct proclaim_is_uint<unsigned long long> : std::true_type {};
    template<> struct proclaim_is_uint<char> : std::conditional_t<std::is_unsigned_v<char>, std::true_type, std::false_type> {};
    template<> struct proclaim_is_uint<wchar_t> : std::conditional_t<std::is_unsigned_v<wchar_t>, std::true_type, std::false_type> {};
    template<> struct proclaim_is_uint<char16_t> : std::conditional_t<std::is_unsigned_v<char16_t>, std::true_type, std::false_type> {};
    template<> struct proclaim_is_uint<char32_t> : std::conditional_t<std::is_unsigned_v<char32_t>, std::true_type, std::false_type> {};
    template<typename T> using is_uint = std::bool_constant<proclaim_is_uint<T>::value>;
    template<typename T> constexpr bool is_uint_v = is_uint<std::decay_t<T>>::value;
    template<typename... Ts> using are_uint = bool_and<is_uint<Ts>::value...>;
    template<typename... Ts> constexpr bool are_uint_v = are_uint<std::decay_t<Ts>...>::value;

    // any signed integer
    template<typename> struct proclaim_is_sint : std::false_type {};
    template<> struct proclaim_is_sint<signed char> : std::true_type {};
    template<> struct proclaim_is_sint<short> : std::true_type {};
    template<> struct proclaim_is_sint<int> : std::true_type {};
    template<> struct proclaim_is_sint<long> : std::true_type {};
    template<> struct proclaim_is_sint<long long> : std::true_type {};
    template<> struct proclaim_is_sint<char> : std::conditional_t<std::is_signed_v<char>, std::true_type, std::false_type> {};
    template<> struct proclaim_is_sint<wchar_t> : std::conditional_t<std::is_signed_v<wchar_t>, std::true_type, std::false_type> {};
    template<> struct proclaim_is_sint<char16_t> : std::conditional_t<std::is_signed_v<char16_t>, std::true_type, std::false_type> {};
    template<> struct proclaim_is_sint<char32_t> : std::conditional_t<std::is_signed_v<char32_t>, std::true_type, std::false_type> {};
    template<typename T> using is_sint = std::bool_constant<proclaim_is_sint<T>::value>;
    template<typename T> constexpr bool is_sint_v = is_sint<std::decay_t<T>>::value;
    template<typename... Ts> using are_sint = bool_and<is_sint<Ts>::value...>;
    template<typename... Ts> constexpr bool are_sint_v = are_sint<std::decay_t<Ts>...>::value;

    // any integer
    template<typename T> using is_int = std::bool_constant<is_uint<T>::value || is_sint<T>::value>;
    template<typename T> constexpr bool is_int_v = is_int<std::decay_t<T>>::value;
    template<typename... Ts> using are_int = bool_and<is_int<Ts>::value...>;
    template<typename... Ts> constexpr bool are_int_v = are_int<std::decay_t<Ts>...>::value;

    // float or double
    template<typename> struct proclaim_is_real : std::false_type {}; // Half is proclaimed in core/types/Half.hpp
    template<> struct proclaim_is_real<float> : std::true_type {};
    template<> struct proclaim_is_real<double> : std::true_type {};
    template<typename T> using is_real = std::bool_constant<proclaim_is_real<T>::value>;
    template<typename T> constexpr bool is_real_v = is_real<std::decay_t<T>>::value;
    template<typename... Ts> using are_real = bool_and<is_real<Ts>::value...>;
    template<typename... Ts> constexpr bool are_real_v = are_real<std::decay_t<Ts>...>::value;

    // std::complex<float|double>
    template<typename> struct proclaim_is_std_complex : std::false_type {};
    template<typename T> using is_std_complex = std::bool_constant<proclaim_is_std_complex<T>::value>;
    template<typename T> constexpr bool is_std_complex_v = is_std_complex<std::decay_t<T>>::value;
    template<typename... Ts> using are_std_complex = bool_and<is_std_complex<Ts>::value...>;
    template<typename... Ts> constexpr bool are_std_complex_v = are_std_complex<std::decay_t<Ts>...>::value;

    // Complex<>
    template<typename> struct proclaim_is_complex : std::false_type {}; // Complex<> is proclaimed in core/types/Complex.hpp
    template<typename T> using is_complex = std::bool_constant<proclaim_is_complex<T>::value>;
    template<typename T> constexpr bool is_complex_v = is_complex<std::decay_t<T>>::value;
    template<typename... Ts> using are_complex = bool_and<is_complex<Ts>::value...>;
    template<typename... Ts> constexpr bool are_complex_v = are_complex<std::decay_t<Ts>...>::value;

    // (complex) floating-point
    template<typename T> using is_real_or_complex = std::bool_constant<is_real<T>::value || is_complex<T>::value>;
    template<typename T> constexpr bool is_real_or_complex_v = is_real_or_complex<std::decay_t<T>>::value;
    template<typename... Ts> using are_real_or_complex = bool_and<is_real_or_complex<Ts>::value...>;
    template<typename... Ts> constexpr bool are_real_or_complex_v = are_real_or_complex<std::decay_t<Ts>...>::value;

    // any integer or real
    template<typename T> using is_scalar = std::bool_constant<is_real<T>::value || is_int<T>::value>;
    template<typename T> constexpr bool is_scalar_v = is_scalar<std::decay_t<T>>::value;
    template<typename... Ts> using are_scalar = bool_and<is_scalar<Ts>::value...>;
    template<typename... Ts> constexpr bool are_scalar_v = are_scalar<std::decay_t<Ts>...>::value;

    // any integer, floating-point or complex floating-point
    template<typename T> using is_numeric = std::bool_constant<is_int<T>::value || is_real_or_complex<T>::value>;
    template<typename T> constexpr bool is_numeric_v = is_numeric<std::decay_t<T>>::value;
    template<typename... Ts> using are_numeric = bool_and<is_numeric<Ts>::value...>;
    template<typename... Ts> constexpr bool are_numeric_v = are_numeric<std::decay_t<Ts>...>::value;
}

namespace noa::traits {
    // -- Accessor --
    template<typename T> struct proclaim_is_accessor : std::false_type {};
    template<typename T> using is_accessor = std::bool_constant<proclaim_is_accessor<T>::value>;
    template<typename T> constexpr bool is_accessor_v = is_accessor<std::decay_t<T>>::value;
    template<typename... Ts> using are_accessor = bool_and<is_accessor<Ts>::value...>::value;
    template<typename... Ts> constexpr bool are_accessor_v = are_accessor<std::decay_t<Ts>...>::value;

    template<typename T> struct proclaim_is_accessor_restrict : std::false_type {};
    template<typename T> using is_accessor_restrict = std::bool_constant<proclaim_is_accessor_restrict<T>::value>;
    template<typename T> constexpr bool is_accessor_restrict_v = is_accessor_restrict<std::decay_t<T>>::value;
    template<typename... Ts> using are_accessor_restrict = bool_and<is_accessor_restrict<Ts>::value...>::value;
    template<typename... Ts> constexpr bool are_accessor_restrict_v = are_accessor_restrict<std::decay_t<Ts>...>::value;

    template<typename T> struct proclaim_is_accessor_contiguous : std::false_type {};
    template<typename T> using is_accessor_contiguous = std::bool_constant<proclaim_is_accessor_contiguous<T>::value>;
    template<typename T> constexpr bool is_accessor_contiguous_v = is_accessor_contiguous<std::decay_t<T>>::value;
    template<typename... Ts> using are_accessor_contiguous = bool_and<is_accessor_contiguous<Ts>::value...>::value;
    template<typename... Ts> constexpr bool are_accessor_contiguous_v = are_accessor_contiguous<std::decay_t<Ts>...>::value;

    template<typename T, size_t N> struct proclaim_is_accessor_nd : std::false_type {};
    template<typename T, size_t N> using is_accessor_nd = std::bool_constant<proclaim_is_accessor_nd<T, N>::value>;
    template<typename T, size_t N> constexpr bool is_accessor_nd_v = is_accessor_nd<std::decay_t<T>, N>::value;
    template<size_t N, typename... Ts> using are_accessor_nd = bool_and<is_accessor_nd<Ts, N>::value...>::value;
    template<size_t N, typename... Ts> constexpr bool are_accessor_nd_v = are_accessor_nd<N, std::decay_t<Ts>...>::value;

    template<typename T> constexpr bool is_accessor_1d_v = is_accessor_nd_v<T, 1>;
    template<typename T> constexpr bool is_accessor_2d_v = is_accessor_nd_v<T, 2>;
    template<typename T> constexpr bool is_accessor_3d_v = is_accessor_nd_v<T, 3>;
    template<typename T> constexpr bool is_accessor_4d_v = is_accessor_nd_v<T, 4>;

    template<typename T> struct proclaim_is_accessor_reference : std::false_type {};
    template<typename T> using is_accessor_reference = std::bool_constant<proclaim_is_accessor_reference<T>::value>;
    template<typename T> constexpr bool is_accessor_reference_v = is_accessor_reference<std::decay_t<T>>::value;
    template<typename... Ts> using are_accessor_reference = bool_and<is_accessor_reference<Ts>::value...>::value;
    template<typename... Ts> constexpr bool are_accessor_reference_v = are_accessor_reference<std::decay_t<Ts>...>::value;

    template<typename T> struct proclaim_is_accessor_value : std::false_type {};
    template<typename T> using is_accessor_value = std::bool_constant<proclaim_is_accessor_value<T>::value>;
    template<typename T> constexpr bool is_accessor_value_v = is_accessor_value<std::decay_t<T>>::value;
    template<typename... Ts> using are_accessor_value = bool_and<is_accessor_value<Ts>::value...>::value;
    template<typename... Ts> constexpr bool are_accessor_value_v = are_accessor_value<std::decay_t<Ts>...>::value;

    template<typename T> constexpr bool is_accessor_1d_restrict_contiguous_v =
            is_accessor_nd_v<T, 1> && is_accessor_restrict_v<T> && is_accessor_contiguous_v<T>;

    // -- Matrices --
    template<typename> struct proclaim_is_mat22 : std::false_type {};
    template<typename T> using is_mat22 = std::bool_constant<proclaim_is_mat22<remove_ref_cv_t<T>>::value>;
    template<typename T> constexpr bool is_mat22_v = is_mat22<T>::value;

    template<typename> struct proclaim_is_mat23 : std::false_type {};
    template<typename T> using is_mat23 = std::bool_constant<proclaim_is_mat23<remove_ref_cv_t<T>>::value>;
    template<typename T> constexpr bool is_mat23_v = is_mat23<T>::value;

    template<typename> struct proclaim_is_mat33 : std::false_type {};
    template<typename T> using is_mat33 = std::bool_constant<proclaim_is_mat33<remove_ref_cv_t<T>>::value>;
    template<typename T> constexpr bool is_mat33_v = is_mat33<T>::value;

    template<typename> struct proclaim_is_mat34 : std::false_type {};
    template<typename T> using is_mat34 = std::bool_constant<proclaim_is_mat34<remove_ref_cv_t<T>>::value>;
    template<typename T> constexpr bool is_mat34_v = is_mat34<T>::value;

    template<typename> struct proclaim_is_mat44 : std::false_type {};
    template<typename T> using is_mat44 = std::bool_constant<proclaim_is_mat44<remove_ref_cv_t<T>>::value>;
    template<typename T> constexpr bool is_mat44_v = is_mat44<T>::value;

    template<typename T> using is_matXX = std::bool_constant<is_mat22_v<T> || is_mat23_v<T> || is_mat33_v<T> || is_mat34_v<T> || is_mat44_v<T>>;
    template<typename T> constexpr bool is_matXX_v = is_matXX<T>::value;

    // -- Vec --
    template<typename T> struct proclaim_is_vec : std::false_type {};
    template<typename T> using is_vec = std::bool_constant<proclaim_is_vec<T>::value>;
    template<typename T> constexpr bool is_vec_v = is_vec<std::decay_t<T>>::value;
    template<typename T> constexpr bool is_vecX_v = is_vec<std::decay_t<T>>::value;

    template<typename, typename> struct proclaim_is_vec_of_type : std::false_type {};
    template<typename T, typename V> using is_vec_of_type = std::bool_constant<proclaim_is_vec_of_type<T, V>::value>;
    template<typename T, typename V> constexpr bool is_vec_of_type_v = is_vec_of_type<std::decay_t<T>, V>::value;
    template<typename T, typename V> constexpr bool is_vecT_v = is_vec_of_type<std::decay_t<T>, V>::value;

    template<typename, size_t> struct proclaim_is_vec_of_size : std::false_type {};
    template<typename T, size_t N> using is_vec_of_size = std::bool_constant<proclaim_is_vec_of_size<T, N>::value>;
    template<typename T, size_t N> constexpr bool is_vec_of_size_v = is_vec_of_size<std::decay_t<T>, N>::value;
    template<typename T, size_t N> constexpr bool is_vecN_v = is_vec_of_size<std::decay_t<T>, N>::value;

    template<typename T> constexpr bool is_vec1_v = is_vecN_v<T, 1>;
    template<typename T> constexpr bool is_vec2_v = is_vecN_v<T, 2>;
    template<typename T> constexpr bool is_vec3_v = is_vecN_v<T, 3>;
    template<typename T> constexpr bool is_vec4_v = is_vecN_v<T, 4>;

    template<typename T> constexpr bool is_intX_v = is_vecX_v<T> && is_int_v<value_type_t<T>>;
    template<typename T, size_t N> constexpr bool is_intN_v = is_vecN_v<T, N> && is_int_v<value_type_t<T>>;
    template<typename T> constexpr bool is_int1_v = is_intN_v<T, 1>;
    template<typename T> constexpr bool is_int2_v = is_intN_v<T, 2>;
    template<typename T> constexpr bool is_int3_v = is_intN_v<T, 3>;
    template<typename T> constexpr bool is_int4_v = is_intN_v<T, 4>;

    template<typename T> constexpr bool is_boolX_v = is_vecX_v<T> && is_bool_v<value_type_t<T>>;
    template<typename T, size_t N> constexpr bool is_boolN_v = is_vecN_v<T, N> && is_bool_v<value_type_t<T>>;
    template<typename T> constexpr bool is_bool1_v = is_boolN_v<T, 1>;
    template<typename T> constexpr bool is_bool2_v = is_boolN_v<T, 2>;
    template<typename T> constexpr bool is_bool3_v = is_boolN_v<T, 3>;
    template<typename T> constexpr bool is_bool4_v = is_boolN_v<T, 4>;

    template<typename T> constexpr bool is_sintX_v = is_vecX_v<T> && is_sint_v<value_type_t<T>>;
    template<typename T, size_t N> constexpr bool is_sintN_v = is_vecN_v<T, N> && is_sint_v<value_type_t<T>>;
    template<typename T> constexpr bool is_sint1_v = is_sintN_v<T, 1>;
    template<typename T> constexpr bool is_sint2_v = is_sintN_v<T, 2>;
    template<typename T> constexpr bool is_sint3_v = is_sintN_v<T, 3>;
    template<typename T> constexpr bool is_sint4_v = is_sintN_v<T, 4>;

    template<typename T> constexpr bool is_uintX_v = is_vecX_v<T> && is_uint_v<value_type_t<T>>;
    template<typename T, size_t N> constexpr bool is_uintN_v = is_vecN_v<T, N> && is_uint_v<value_type_t<T>>;
    template<typename T> constexpr bool is_uint1_v = is_uintN_v<T, 1>;
    template<typename T> constexpr bool is_uint2_v = is_uintN_v<T, 2>;
    template<typename T> constexpr bool is_uint3_v = is_uintN_v<T, 3>;
    template<typename T> constexpr bool is_uint4_v = is_uintN_v<T, 4>;

    template<typename T> constexpr bool is_realX_v = is_vecX_v<T> && is_real_v<value_type_t<T>>;
    template<typename T, size_t N> constexpr bool is_realN_v = is_vecN_v<T, N> && is_real_v<value_type_t<T>>;
    template<typename T> constexpr bool is_real1_v = is_realN_v<T, 1>;
    template<typename T> constexpr bool is_real2_v = is_realN_v<T, 2>;
    template<typename T> constexpr bool is_real3_v = is_realN_v<T, 3>;
    template<typename T> constexpr bool is_real4_v = is_realN_v<T, 4>;

    // -- Shape and Strides --
    template<typename T> struct proclaim_is_shape : std::false_type {};
    template<typename T> using is_shape = std::bool_constant<proclaim_is_shape<T>::value>;
    template<typename T> constexpr bool is_shape_v = is_shape<std::decay_t<T>>::value;
    template<typename T> constexpr bool is_shapeX_v = is_shape<std::decay_t<T>>::value;

    template<typename, typename> struct proclaim_is_shape_of_type : std::false_type {};
    template<typename T, typename V> using is_shape_of_type = std::bool_constant<proclaim_is_shape_of_type<T, V>::value>;
    template<typename T, typename V> constexpr bool is_shape_of_type_v = is_shape_of_type<std::decay_t<T>, V>::value;
    template<typename T, typename V> constexpr bool is_shapeT_v = is_shape_of_type<std::decay_t<T>, V>::value;

    template<typename, size_t> struct proclaim_is_shape_of_size : std::false_type {};
    template<typename T, size_t N> using is_shape_of_size = std::bool_constant<proclaim_is_shape_of_size<T, N>::value>;
    template<typename T, size_t N> constexpr bool is_shape_of_size_v = is_shape_of_size<std::decay_t<T>, N>::value;
    template<typename T, size_t N> constexpr bool is_shapeN_v = is_shape_of_size<std::decay_t<T>, N>::value;

    template<typename T> constexpr bool is_shape1_v = is_shapeN_v<T, 1>;
    template<typename T> constexpr bool is_shape2_v = is_shapeN_v<T, 2>;
    template<typename T> constexpr bool is_shape3_v = is_shapeN_v<T, 3>;
    template<typename T> constexpr bool is_shape4_v = is_shapeN_v<T, 4>;

    template<typename T> struct proclaim_is_strides : std::false_type {};
    template<typename T> using is_strides = std::bool_constant<proclaim_is_strides<T>::value>;
    template<typename T> constexpr bool is_strides_v = is_strides<std::decay_t<T>>::value;
    template<typename T> constexpr bool is_stridesX_v = is_strides<std::decay_t<T>>::value;

    template<typename, typename> struct proclaim_is_strides_of_type : std::false_type {};
    template<typename T, typename V> using is_strides_of_type = std::bool_constant<proclaim_is_strides_of_type<T, V>::value>;
    template<typename T, typename V> constexpr bool is_strides_of_type_v = is_strides_of_type<std::decay_t<T>, V>::value;
    template<typename T, typename V> constexpr bool is_stridesT_v = is_strides_of_type<std::decay_t<T>, V>::value;

    template<typename, size_t> struct proclaim_is_strides_of_size : std::false_type {};
    template<typename T, size_t N> using is_strides_of_size = std::bool_constant<proclaim_is_strides_of_size<T, N>::value>;
    template<typename T, size_t N> constexpr bool is_strides_of_size_v = is_strides_of_size<std::decay_t<T>, N>::value;
    template<typename T, size_t N> constexpr bool is_stridesN_v = is_strides_of_size<std::decay_t<T>, N>::value;

    template<typename T> constexpr bool is_strides1_v = is_stridesN_v<T, 1>;
    template<typename T> constexpr bool is_strides2_v = is_stridesN_v<T, 2>;
    template<typename T> constexpr bool is_strides3_v = is_stridesN_v<T, 3>;
    template<typename T> constexpr bool is_strides4_v = is_stridesN_v<T, 4>;

    template<typename T> constexpr bool is_shape_or_strides_v = is_strides_v<T> || is_shape_v<T>;
    template<typename T, size_t N> constexpr bool is_shapeN_or_stridesN_v = is_stridesN_v<T, N> || is_shapeN_v<T, N>;

    // -- Tuple --
    template<typename T> struct proclaim_is_tuple : std::false_type {};
    template<typename T> using is_tuple = std::bool_constant<proclaim_is_tuple<T>::value>;
    template<typename T> constexpr bool is_tuple_v = is_tuple<std::decay_t<T>>::value;
    template<typename... Ts> using are_tuple = bool_and<is_tuple<Ts>::value...>::value;
    template<typename... Ts> constexpr bool are_tuple_v = are_tuple<std::decay_t<Ts>...>::value;

    template<typename T> struct proclaim_is_tuple_of_accessor : std::false_type {};
    template<typename T> using is_tuple_of_accessor = std::bool_constant<proclaim_is_tuple_of_accessor<T>::value>;
    template<typename T> constexpr bool is_tuple_of_accessor_v = is_tuple_of_accessor<std::decay_t<T>>::value;
    template<typename... Ts> using are_tuple_of_accessor = bool_and<is_tuple_of_accessor<Ts>::value...>::value;
    template<typename... Ts> constexpr bool are_tuple_of_accessor_v = are_tuple_of_accessor<std::decay_t<Ts>...>::value;
}
