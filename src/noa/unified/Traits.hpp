#pragma once

#include "noa/core/Traits.hpp"

namespace noa::traits {
    NOA_GENERATE_PROCLAIM_FULL(array);
    NOA_GENERATE_PROCLAIM_FULL(view);

    template<typename T> using is_array_decay = is_array<std::decay_t<T>>;
    NOA_GENERATE_PROCLAIM_UTILS(array_decay);
    template<typename... T> concept array_decay = are_array_decay_v<T...>;

    template<typename T> using is_view_decay = is_view<std::decay_t<T>>;
    NOA_GENERATE_PROCLAIM_UTILS(view_decay);
    template<typename... T> concept view_decay = are_view_decay_v<T...>;

    template<typename T> using is_varray = std::disjunction<is_array<T>, is_view<T>>;
    NOA_GENERATE_PROCLAIM_UTILS(varray);
    template<typename... T> concept varray = are_varray_v<T...>;

    template<typename T> using is_varray_decay = is_varray<std::decay_t<T>>;
    NOA_GENERATE_PROCLAIM_UTILS(varray_decay);
    template<typename... T> concept varray_decay = are_varray_decay_v<T...>;

    template<typename... T> concept readable_varray = varray<T...> and ((not std::is_void_v<value_type_t<T>>) and ...);
    template<typename... T> concept writable_varray = readable_varray<T...> and ((not std::is_const_v<value_type_t<T>>) and ...);
    template<typename... T> concept readable_varray_decay = varray_decay<T...> and ((not std::is_void_v<value_type_t<T>>) and ...);
    template<typename... T> concept writable_varray_decay = readable_varray_decay<T...> and ((not std::is_const_v<value_type_t<T>>) and ...);

    #define NOA_TRAITS_GENERATE_VARRAY(name)                                                                                    \
    template<typename... T> concept varray_of_##name = varray<T...> and name<value_type_t<T>...>;                               \
    template<typename... T> concept varray_decay_of_##name = varray_decay<T...> and name<value_type_t<T>...>;                   \
    template<typename... T> concept readable_varray_of_##name = readable_varray<T...> and name<value_type_t<T>...>;             \
    template<typename... T> concept writable_varray_of_##name = writable_varray<T...> and name<value_type_t<T>...>;             \
    template<typename... T> concept readable_varray_decay_of_##name = readable_varray_decay<T...> and name<value_type_t<T>...>; \
    template<typename... T> concept writable_varray_decay_of_##name = writable_varray_decay<T...> and name<value_type_t<T>...>
    NOA_TRAITS_GENERATE_VARRAY(integer);
    NOA_TRAITS_GENERATE_VARRAY(sinteger);
    NOA_TRAITS_GENERATE_VARRAY(uinteger);
    NOA_TRAITS_GENERATE_VARRAY(real);
    NOA_TRAITS_GENERATE_VARRAY(complex);
    NOA_TRAITS_GENERATE_VARRAY(real_or_complex);
    NOA_TRAITS_GENERATE_VARRAY(scalar);
    NOA_TRAITS_GENERATE_VARRAY(numeric);
    NOA_TRAITS_GENERATE_VARRAY(boolean);
    #undef NOA_TRAITS_GENERATE_VARRAY

    #define NOA_TRAITS_GENERATE_VARRAY_SAME_AS(suffix, constrain)                                                                                               \
    template<typename T, typename... U> concept varray_of_##suffix = varray<T> and constrain<value_type_t<T>, U...>;                                            \
    template<typename T, typename... U> concept varray_decay_of_##suffix = varray_decay<T> and constrain<value_type_t<T>, U...>;                                \
    template<typename T, typename... U> concept readable_varray_decay_of_##suffix = readable_varray_decay<T> and constrain<value_type_t<T>, U...>;              \
    template<typename T, typename... U> concept writable_varray_decay_of_##suffix = writable_varray_decay<T> and constrain<value_type_t<T>, U...>;              \
    template<typename T, typename... U> concept varray_or_value_of_##suffix = varray_of_##suffix<T, U...> or constrain<T, U...>;                                \
    template<typename T, typename... U> concept varray_decay_or_value_of_##suffix = varray_decay_of_##suffix<T, U...> or constrain<T, U...>;                    \
    template<typename T, typename... U> concept readable_varray_decay_or_value_of_##suffix = readable_varray_decay_of_##suffix<T, U...> or constrain<T, U...>;  \
    template<typename T, typename... U> concept writable_varray_decay_or_value_of_##suffix = writable_varray_decay_of_##suffix<T, U...> or constrain<T, U...>

    NOA_TRAITS_GENERATE_VARRAY_SAME_AS(any, any_of);
    NOA_TRAITS_GENERATE_VARRAY_SAME_AS(almost_any, almost_any_of);
    #undef NOA_TRAITS_GENERATE_VARRAY_SAME_AS

    template<typename T, typename... U> concept varray_of_almost_same_type = varray<T, U...> and almost_same_as<value_type_t<T>, value_type_t<U>...>;
    template<typename T, typename... U> concept varray_decay_of_almost_same_type = varray_decay<T, U...> and almost_same_as<value_type_t<T>, value_type_t<std::decay_t<U>>...>;
    template<typename T, typename... U> concept readable_varray_decay_of_almost_same_type = readable_varray_decay<T, U...> and almost_same_as<value_type_t<T>, value_type_t<std::decay_t<U>>...>;
    template<typename T, typename... U> concept writable_varray_decay_of_almost_same_type = writable_varray_decay<T, U...> and almost_same_as<value_type_t<T>, value_type_t<std::decay_t<U>>...>;

    template<typename T, typename... U> concept varray_with_compatible_types = varray<T, U...> and (compatible_types<value_type_t<T>, value_type_t<U>> and ...);
    template<typename T, typename... U> concept varray_decay_with_compatible_types = varray_decay<T, U...> and (compatible_types<value_type_t<T>, value_type_t<U>> and ...);

    template<typename T, typename... U> concept varray_with_spectrum_types = varray<T, U...> and (spectrum_types<value_type_t<T>, value_type_t<U>> and ...);
    template<typename T, typename... U> concept varray_decay_with_spectrum_types = varray_decay<T, U...> and (spectrum_types<value_type_t<T>, value_type_t<U>> and ...);

    template<typename T, typename... U> concept varray_with_compatible_or_spectrum_types = varray<T, U...> and (compatible_or_spectrum_types<value_type_t<T>, value_type_t<U>> and ...);
    template<typename T, typename... U> concept varray_decay_with_compatible_or_spectrum_types = varray_decay<T, U...> and (compatible_or_spectrum_types<value_type_t<T>, value_type_t<U>> and ...);

    template<typename T, typename U> concept varray_compatible_with = varray<T> and compatible_types<value_type_t<T>, value_type_t<U>>;
    template<typename T, typename U> concept varray_decay_compatible_with = varray_decay<T> and compatible_types<value_type_t<T>, value_type_t<U>>;
}

namespace noa::traits {
    NOA_GENERATE_PROCLAIM_FULL(texture);

    template<typename T> using is_texture_decay = is_texture<std::decay_t<T>>;
    NOA_GENERATE_PROCLAIM_UTILS(texture_decay);
    template<typename... T> concept texture_decay = are_texture_decay_v<T...>;

    #define NOA_TRAITS_GENERATE_TEXTURE(name)                                                           \
    template<typename... T> concept texture_of_##name = texture<T...> and name<value_type_t<T>...>;     \
    template<typename... T> concept texture_decay_of_##name = texture_decay<T...> and name<value_type_t<T>...>
    NOA_TRAITS_GENERATE_TEXTURE(real);
    NOA_TRAITS_GENERATE_TEXTURE(complex);
    NOA_TRAITS_GENERATE_TEXTURE(real_or_complex);
    #undef NOA_TRAITS_GENERATE_TEXTURE

    #define NOA_TRAITS_GENERATE_TEXTURE_SAME_AS(suffix, constrain)                                                                  \
    template<typename T, typename... U> concept texture_of_##suffix = texture<T> and constrain<value_type_t<T>, U...>;              \
    template<typename T, typename... U> concept texture_decay_of_##suffix = texture_decay<T> and constrain<value_type_t<T>, U...>;  \
    template<typename T, typename... U> concept texture_or_value_of_##suffix = texture_of_##suffix<T, U...> or constrain<T, U...>;  \
    template<typename T, typename... U> concept texture_decay_or_value_of_##suffix = texture_decay_of_##suffix<T, U...> or constrain<T, U...>
    NOA_TRAITS_GENERATE_TEXTURE_SAME_AS(any, any_of);
    NOA_TRAITS_GENERATE_TEXTURE_SAME_AS(almost_any, almost_any_of);
    #undef NOA_TRAITS_GENERATE_TEXTURE_SAME_AS

    template<typename... T>
    concept varray_or_texture = ((varray<T> or texture<T>) and ...);

    template<typename... T>
    concept varray_or_texture_decay = ((varray_decay<T> or texture_decay<T>) and ...);

    template<typename T, typename... U>
    concept varray_or_texture_with_spectrum_types =
        (varray<T, U...> or (texture<T> and varray<U...>)) and
        (spectrum_types<value_type_t<T>, value_type_t<U>> and ...);

    template<typename T, typename... U>
    concept varray_or_texture_decay_with_spectrum_types =
        (varray_decay<T, U...> or (texture_decay<T> and varray_decay<U...>)) and
        (spectrum_types<value_type_t<T>, value_type_t<U>> and ...);

    #define NOA_TRAITS_GENERATE_VARRAY_OR_TEXTURE(name)                                                         \
    template<typename... T> concept varray_or_texture_of_##name = varray_or_texture<T...> and name<value_type_t<T>...>;    \
    template<typename... T> concept varray_or_texture_decay_of_##name = varray_or_texture_decay<T...> and name<value_type_t<T>...>
    NOA_TRAITS_GENERATE_VARRAY_OR_TEXTURE(real);
    NOA_TRAITS_GENERATE_VARRAY_OR_TEXTURE(complex);
    NOA_TRAITS_GENERATE_VARRAY_OR_TEXTURE(real_or_complex);
    #undef NOA_TRAITS_GENERATE_VARRAY_OR_TEXTURE

    template<typename T, size_t N, typename U = std::remove_reference_t<T>, typename V = value_type_t<T>>
    concept transform_affine_nd =
        mat_of_shape<U, N, N + 1> or
        mat_of_shape<U, N + 1, N + 1> or
        (nt::varray<U> and (mat_of_shape<V, N, N + 1> or mat_of_shape<V, N + 1, N + 1>));

    template<typename T, size_t N, typename U = std::remove_reference_t<T>, typename V = value_type_t<T>>
    concept transform_projection_nd =
        mat_of_shape<U, N - 1, N + 1> or
        mat_of_shape<U, N + 1, N + 1> or
        (nt::varray<U> and (mat_of_shape<V, N - 1, N + 1> or mat_of_shape<V, N + 1, N + 1>));
}
