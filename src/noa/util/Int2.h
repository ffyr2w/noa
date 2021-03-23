/**
 * @file noa/util/Int2.h
 * @author Thomas - ffyr2w
 * @date 10/12/2020
 */
#pragma once

#include <string>
#include <array>
#include <type_traits>
#include <spdlog/fmt/fmt.h>

#include "noa/Definitions.h"
#include "noa/Math.h"
#include "noa/util/traits/BaseTypes.h"
#include "noa/util/string/Format.h"

namespace Noa {
    template<typename>
    struct Float2;

    template<typename T>
    struct Int2 {
        std::enable_if_t<Noa::Traits::is_int_v<T>, T> x{0}, y{0};

        // Constructors.
        NOA_HD constexpr Int2() = default;
        NOA_HD constexpr Int2(T xi, T yi) : x(xi), y(yi) {}
        NOA_HD constexpr explicit Int2(T v) : x(v), y(v) {}
        NOA_HD constexpr explicit Int2(T* ptr) : x(ptr[0]), y(ptr[1]) {}

        // Conversion constructors.
        template<typename U, typename = std::enable_if_t<Noa::Traits::is_scalar_v<U>>>
        NOA_HD constexpr explicit Int2(U* ptr) : x(T(ptr[0])), y(T(ptr[1])) {}

        template<typename U>
        NOA_HD constexpr explicit Int2(Int2<U> vec) : x(T(vec.x)), y(T(vec.y)) {}

        template<typename U>
        NOA_HD constexpr explicit Int2(Float2<U> vec) : x(T(vec.x)), y(T(vec.y)) {}

        // Assignment operators.
        NOA_HD constexpr Int2<T>& operator=(T v) noexcept {
            x = v;
            y = v;
            return *this;
        }

        NOA_HD constexpr Int2<T>& operator=(T* ptr) noexcept {
            x = ptr[0];
            y = ptr[1];
            return *this;
        }

        template<typename U, typename = std::enable_if_t<Noa::Traits::is_scalar_v<U>>>
        NOA_HD constexpr auto& operator=(U* ptr) noexcept {
            x = T(ptr[0]);
            y = T(ptr[1]);
            return *this;
        }

        template<typename U>
        NOA_HD constexpr auto& operator=(Int2<U> vec) noexcept {
            x = T(vec.x);
            y = T(vec.y);
            return *this;
        }

        template<typename U>
        NOA_HD constexpr auto& operator=(Float2<U> vec) noexcept {
            x = T(vec.x);
            y = T(vec.y);
            return *this;
        }

        [[nodiscard]] NOA_HD static constexpr size_t size() noexcept { return 2U; }
        [[nodiscard]] NOA_HOST constexpr std::array<T, 2U> toArray() const noexcept { return {x, y}; }
        [[nodiscard]] NOA_HOST std::string toString() const { return String::format("({},{})", x, y); }

        NOA_HD constexpr Int2<T>& operator+=(const Int2<T>& rhs) noexcept;
        NOA_HD constexpr Int2<T>& operator-=(const Int2<T>& rhs) noexcept;
        NOA_HD constexpr Int2<T>& operator*=(const Int2<T>& rhs) noexcept;
        NOA_HD constexpr Int2<T>& operator/=(const Int2<T>& rhs) noexcept;

        NOA_HD constexpr Int2<T>& operator+=(T rhs) noexcept;
        NOA_HD constexpr Int2<T>& operator-=(T rhs) noexcept;
        NOA_HD constexpr Int2<T>& operator*=(T rhs) noexcept;
        NOA_HD constexpr Int2<T>& operator/=(T rhs) noexcept;
    };

    using int2_t = Int2<int>;
    using uint2_t = Int2<uint>;
    using long2_t = Int2<long long>;
    using ulong2_t = Int2<unsigned long long>;

    template<typename T>
    [[nodiscard]] NOA_IH std::string toString(const Int2<T>& v) { return v.toString(); }

    /* --- Binary Arithmetic Operators --- */

    template<typename I>
    NOA_FHD constexpr Int2<I> operator+(Int2<I> lhs, Int2<I> rhs) noexcept {
        return {lhs.x + rhs.x, lhs.y + rhs.y};
    }
    template<typename I>
    NOA_FHD constexpr Int2<I> operator+(I lhs, Int2<I> rhs) noexcept {
        return {lhs + rhs.x, lhs + rhs.y};
    }
    template<typename I>
    NOA_FHD constexpr Int2<I> operator+(Int2<I> lhs, I rhs) noexcept {
        return {lhs.x + rhs, lhs.y + rhs};
    }

    template<typename I>
    NOA_FHD constexpr Int2<I> operator-(Int2<I> lhs, Int2<I> rhs) noexcept {
        return {lhs.x - rhs.x, lhs.y - rhs.y};
    }
    template<typename I>
    NOA_FHD constexpr Int2<I> operator-(I lhs, Int2<I> rhs) noexcept {
        return {lhs - rhs.x, lhs - rhs.y};
    }
    template<typename I>
    NOA_FHD constexpr Int2<I> operator-(Int2<I> lhs, I rhs) noexcept {
        return {lhs.x - rhs, lhs.y - rhs};
    }

    template<typename I>
    NOA_FHD constexpr Int2<I> operator*(Int2<I> lhs, Int2<I> rhs) noexcept {
        return {lhs.x * rhs.x, lhs.y * rhs.y};
    }
    template<typename I>
    NOA_FHD constexpr Int2<I> operator*(I lhs, Int2<I> rhs) noexcept {
        return {lhs * rhs.x, lhs * rhs.y};
    }
    template<typename I>
    NOA_FHD constexpr Int2<I> operator*(Int2<I> lhs, I rhs) noexcept {
        return {lhs.x * rhs, lhs.y * rhs};
    }

    template<typename I>
    NOA_FHD constexpr Int2<I> operator/(Int2<I> lhs, Int2<I> rhs) noexcept {
        return {lhs.x / rhs.x, lhs.y / rhs.y};
    }
    template<typename I>
    NOA_FHD constexpr Int2<I> operator/(I lhs, Int2<I> rhs) noexcept {
        return {lhs / rhs.x, lhs / rhs.y};
    }
    template<typename I>
    NOA_FHD constexpr Int2<I> operator/(Int2<I> lhs, I rhs) noexcept {
        return {lhs.x / rhs, lhs.y / rhs};
    }

    /* --- Binary Arithmetic Assignment Operators --- */

    template<typename I>
    NOA_FHD constexpr Int2<I>& Int2<I>::operator+=(const Int2<I>& rhs) noexcept {
        *this = *this + rhs;
        return *this;
    }
    template<typename I>
    NOA_FHD constexpr Int2<I>& Int2<I>::operator+=(I rhs) noexcept {
        *this = *this + rhs;
        return *this;
    }

    template<typename I>
    NOA_FHD constexpr Int2<I>& Int2<I>::operator-=(const Int2<I>& rhs) noexcept {
        *this = *this - rhs;
        return *this;
    }
    template<typename I>
    NOA_FHD constexpr Int2<I>& Int2<I>::operator-=(I rhs) noexcept {
        *this = *this - rhs;
        return *this;
    }

    template<typename I>
    NOA_FHD constexpr Int2<I>& Int2<I>::operator*=(const Int2<I>& rhs) noexcept {
        *this = *this * rhs;
        return *this;
    }
    template<typename I>
    NOA_FHD constexpr Int2<I>& Int2<I>::operator*=(I rhs) noexcept {
        *this = *this * rhs;
        return *this;
    }

    template<typename I>
    NOA_FHD constexpr Int2<I>& Int2<I>::operator/=(const Int2<I>& rhs) noexcept {
        *this = *this / rhs;
        return *this;
    }
    template<typename I>
    NOA_FHD constexpr Int2<I>& Int2<I>::operator/=(I rhs) noexcept {
        *this = *this / rhs;
        return *this;
    }

    /* --- Comparison Operators --- */

    template<typename I>
    NOA_FHD constexpr bool operator>(const Int2<I>& lhs, const Int2<I>& rhs) noexcept {
        return lhs.x > rhs.x && lhs.y > rhs.y;
    }
    template<typename I>
    NOA_FHD constexpr bool operator>(const Int2<I>& lhs, I rhs) noexcept {
        return lhs.x > rhs && lhs.y > rhs;
    }
    template<typename I>
    NOA_FHD constexpr bool operator>(I lhs, const Int2<I>& rhs) noexcept {
        return lhs > rhs.x && lhs > rhs.y;
    }

    template<typename I>
    NOA_FHD constexpr bool operator<(const Int2<I>& lhs, const Int2<I>& rhs) noexcept {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    }
    template<typename I>
    NOA_FHD constexpr bool operator<(const Int2<I>& lhs, I rhs) noexcept {
        return lhs.x < rhs && lhs.y < rhs;
    }
    template<typename I>
    NOA_FHD constexpr bool operator<(I lhs, const Int2<I>& rhs) noexcept {
        return lhs < rhs.x && lhs < rhs.y;
    }

    template<typename I>
    NOA_FHD constexpr bool operator>=(const Int2<I>& lhs, const Int2<I>& rhs) noexcept {
        return lhs.x >= rhs.x && lhs.y >= rhs.y;
    }
    template<typename I>
    NOA_FHD constexpr bool operator>=(const Int2<I>& lhs, I rhs) noexcept {
        return lhs.x >= rhs && lhs.y >= rhs;
    }
    template<typename I>
    NOA_FHD constexpr bool operator>=(I lhs, const Int2<I>& rhs) noexcept {
        return lhs >= rhs.x && lhs >= rhs.y;
    }

    template<typename I>
    NOA_FHD constexpr bool operator<=(const Int2<I>& lhs, const Int2<I>& rhs) noexcept {
        return lhs.x <= rhs.x && lhs.y <= rhs.y;
    }
    template<typename I>
    NOA_FHD constexpr bool operator<=(const Int2<I>& lhs, I rhs) noexcept {
        return lhs.x <= rhs && lhs.y <= rhs;
    }
    template<typename I>
    NOA_FHD constexpr bool operator<=(I lhs, const Int2<I>& rhs) noexcept {
        return lhs <= rhs.x && lhs <= rhs.y;
    }

    template<typename I>
    NOA_FHD constexpr bool operator==(const Int2<I>& lhs, const Int2<I>& rhs) noexcept {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }
    template<typename I>
    NOA_FHD constexpr bool operator==(const Int2<I>& lhs, I rhs) noexcept {
        return lhs.x == rhs && lhs.y == rhs;
    }
    template<typename I>
    NOA_FHD constexpr bool operator==(I lhs, const Int2<I>& rhs) noexcept {
        return lhs == rhs.x && lhs == rhs.y;
    }

    template<typename I>
    NOA_FHD constexpr bool operator!=(const Int2<I>& lhs, const Int2<I>& rhs) noexcept {
        return !(lhs == rhs);
    }
    template<typename I>
    NOA_FHD constexpr bool operator!=(const Int2<I>& lhs, I rhs) noexcept {
        return !(lhs == rhs);
    }
    template<typename I>
    NOA_FHD constexpr bool operator!=(I lhs, const Int2<I>& rhs) noexcept {
        return !(lhs == rhs);
    }
}

namespace Noa::Math {
    template<class T>
    [[nodiscard]] NOA_FHD constexpr T sum(const Int2<T>& v) noexcept {
        return v.x + v.y;
    }

    template<class T>
    [[nodiscard]] NOA_FHD constexpr T prod(const Int2<T>& v) noexcept {
        return v.x * v.y;
    }

    template<class T>
    [[nodiscard]] NOA_FHD constexpr Int2<T> min(Int2<T> lhs, Int2<T> rhs) {
        return {min(lhs.x, rhs.x), min(lhs.y, rhs.y)};
    }

    template<class T>
    [[nodiscard]] NOA_FHD constexpr Int2<T> min(Int2<T> lhs, T rhs) {
        return {min(lhs.x, rhs), min(lhs.y, rhs)};
    }

    template<class T>
    [[nodiscard]] NOA_FHD constexpr Int2<T> min(T lhs, Int2<T> rhs) {
        return {min(lhs, rhs.x), min(lhs, rhs.y)};
    }

    template<class T>
    [[nodiscard]] NOA_FHD constexpr Int2<T> max(Int2<T> lhs, Int2<T> rhs) {
        return {max(lhs.x, rhs.x), max(lhs.y, rhs.y)};
    }

    template<class T>
    [[nodiscard]] NOA_FHD constexpr Int2<T> max(Int2<T> lhs, T rhs) {
        return {max(lhs.x, rhs), max(lhs.y, rhs)};
    }

    template<class T>
    [[nodiscard]] NOA_FHD constexpr Int2<T> max(T lhs, Int2<T> rhs) {
        return {max(lhs, rhs.x), max(lhs, rhs.y)};
    }
}

namespace Noa {
    template<class T>
    [[nodiscard]] NOA_FHD constexpr size_t getElements(const Int2<T>& v) noexcept {
        return size_t(v.x) * size_t(v.y);
    }

    template<class T>
    [[nodiscard]] NOA_FHD constexpr size_t getElementsFFT(const Int2<T>& v) noexcept {
        return size_t(v.x / 2 + 1) * size_t(v.y);
    }
}

namespace Noa::Traits {
    template<typename> struct p_is_int2 : std::false_type {};
    template<typename T> struct p_is_int2<Noa::Int2<T>> : std::true_type {};
    template<typename T> using is_int2 = std::bool_constant<p_is_int2<Noa::Traits::remove_ref_cv_t<T>>::value>;
    template<typename T> constexpr bool is_int2_v = is_int2<T>::value;

    template<typename> struct p_is_uint2 : std::false_type {};
    template<typename T> struct p_is_uint2<Noa::Int2<T>> : std::bool_constant<Noa::Traits::is_uint_v<T>> {};
    template<typename T> using is_uint2 = std::bool_constant<p_is_uint2<Noa::Traits::remove_ref_cv_t<T>>::value>;
    template<typename T> constexpr bool is_uint2_v = is_uint2<T>::value;

    template<typename T> struct proclaim_is_intX<Noa::Int2<T>> : std::true_type {};
    template<typename T> struct proclaim_is_uintX<Noa::Int2<T>> : std::bool_constant<Noa::Traits::is_uint_v<T>> {};
}

template<typename T>
struct fmt::formatter<T, std::enable_if_t<Noa::Traits::is_int2_v<T>, char>>
        : fmt::formatter<std::string> {
    template<typename FormatCtx>
    auto format(const T& int2, FormatCtx& ctx) {
        return fmt::formatter<std::string>::format(int2.toString(), ctx);
    }
};

template<typename T>
std::ostream& operator<<(std::ostream& os, const Noa::Int2<T>& int2) {
    os << int2.toString();
    return os;
}
