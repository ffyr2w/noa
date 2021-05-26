/**
 * @file noa/util/Bool4.h
 * @author Thomas - ffyr2w
 * @date 10/12/2020
 * Vector containing 4 booleans.
 */
#pragma once

#include <string>
#include <array>
#include <type_traits>

#include "noa/Definitions.h"
#include "noa/util/string/Format.h"
#include "noa/util/traits/BaseTypes.h"

namespace Noa {
    struct Bool4 {
        typedef bool value_type;
        bool x{}, y{}, z{}, w{};

    public: // Component accesses
        [[nodiscard]] NOA_HD static constexpr size_t elements() noexcept { return 4; }
        [[nodiscard]] NOA_HD static constexpr size_t size() noexcept { return elements(); }
        NOA_HD constexpr bool& operator[](size_t i) noexcept;
        NOA_HD constexpr const bool& operator[](size_t i) const noexcept;

    public: // (Conversion) Constructors
        NOA_HD constexpr Bool4() noexcept = default;
        template<typename X, typename Y, typename Z, typename W>
        NOA_HD constexpr Bool4(X xi, Y yi, Z zi, W wi) noexcept;
        template<typename U> NOA_HD constexpr explicit Bool4(U v) noexcept;
        template<typename U> NOA_HD constexpr explicit Bool4(U* ptr);

    public: // Assignment operators
        template<typename U> NOA_HD constexpr Bool4& operator=(U v) noexcept;
        template<typename U> NOA_HD constexpr Bool4& operator=(U* ptr) noexcept;
    };

    // -- Boolean operators --

    NOA_HD constexpr bool operator==(const Bool4& lhs, const Bool4& rhs) noexcept;
    NOA_HD constexpr bool operator==(const Bool4& lhs, bool rhs) noexcept;
    NOA_HD constexpr bool operator==(bool lhs, const Bool4& rhs) noexcept;

    NOA_HD constexpr bool operator!=(const Bool4& lhs, const Bool4& rhs) noexcept;
    NOA_HD constexpr bool operator!=(const Bool4& lhs, bool rhs) noexcept;
    NOA_HD constexpr bool operator!=(bool lhs, const Bool4& rhs) noexcept;

    NOA_HD constexpr Bool4 operator&&(const Bool4& lhs, const Bool4& rhs) noexcept;
    NOA_HD constexpr Bool4 operator||(const Bool4& lhs, const Bool4& rhs) noexcept;

    NOA_FHD constexpr bool any(const Bool4& v) noexcept;
    NOA_FHD constexpr bool all(const Bool4& v) noexcept;

    using bool4_t = Bool4;

    [[nodiscard]] NOA_HOST constexpr std::array<bool, 4> toArray(const Bool4& v) noexcept {
        return {v.x, v.y, v.z, v.w};
    }

    template<> NOA_IH std::string String::typeName<Bool4>() { return "bool4"; }

    NOA_IH std::ostream& operator<<(std::ostream& os, const Bool4& v) {
        os << String::format("({},{},{},{})", v.x, v.y, v.z, v.w);
        return os;
    }

    template<> struct Traits::proclaim_is_boolX<Bool4> : std::true_type {};
}

namespace Noa {
    // -- Component accesses --

    NOA_HD constexpr bool& Bool4::operator[](size_t i) noexcept {
        switch (i) {
            default:
            case 0:
                return this->x;
            case 1:
                return this->y;
            case 2:
                return this->z;
            case 3:
                return this->w;
        }
    }

    NOA_HD constexpr const bool& Bool4::operator[](size_t i) const noexcept {
        switch (i) {
            default:
            case 0:
                return this->x;
            case 1:
                return this->y;
            case 2:
                return this->z;
            case 3:
                return this->w;
        }
    }

    // -- (Conversion) Constructors --

    template<typename X, typename Y, typename Z, typename W>
    NOA_HD constexpr Bool4::Bool4(X xi, Y yi, Z zi, W wi) noexcept
            : x(static_cast<bool>(xi)),
              y(static_cast<bool>(yi)),
              z(static_cast<bool>(zi)),
              w(static_cast<bool>(wi)) {}

    template<typename U>
    NOA_HD constexpr Bool4::Bool4(U v) noexcept
            : x(static_cast<bool>(v)),
              y(static_cast<bool>(v)),
              z(static_cast<bool>(v)),
              w(static_cast<bool>(v)) {}

    template<typename U>
    NOA_HD constexpr Bool4::Bool4(U* ptr)
            : x(static_cast<bool>(ptr[0])),
              y(static_cast<bool>(ptr[1])),
              z(static_cast<bool>(ptr[2])),
              w(static_cast<bool>(ptr[3])) {}

    // -- Assignment operators --

    template<typename U>
    NOA_HD constexpr Bool4& Bool4::operator=(U v) noexcept {
        this->x = static_cast<bool>(v);
        this->y = static_cast<bool>(v);
        this->z = static_cast<bool>(v);
        this->w = static_cast<bool>(v);
        return *this;
    }

    template<typename U>
    NOA_HD constexpr Bool4& Bool4::operator=(U* ptr) noexcept {
        this->x = static_cast<bool>(ptr[0]);
        this->y = static_cast<bool>(ptr[1]);
        this->z = static_cast<bool>(ptr[2]);
        this->z = static_cast<bool>(ptr[3]);
        return *this;
    }

    NOA_FHD constexpr bool operator==(const Bool4& lhs, const Bool4& rhs) noexcept {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
    }
    NOA_FHD constexpr bool operator==(const Bool4& lhs, bool rhs) noexcept {
        return lhs.x == rhs && lhs.y == rhs && lhs.z == rhs && lhs.w == rhs;
    }
    NOA_FHD constexpr bool operator==(bool lhs, const Bool4& rhs) noexcept {
        return lhs == rhs.x && lhs == rhs.y && lhs == rhs.z && lhs == rhs.w;
    }

    NOA_FHD constexpr bool operator!=(const Bool4& lhs, const Bool4& rhs) noexcept {
        return !(lhs == rhs);
    }
    NOA_FHD constexpr bool operator!=(const Bool4& lhs, bool rhs) noexcept {
        return !(lhs == rhs);
    }
    NOA_FHD constexpr bool operator!=(bool lhs, const Bool4& rhs) noexcept {
        return !(lhs == rhs);
    }

    NOA_FHD constexpr Bool4 operator&&(const Bool4& lhs, const Bool4& rhs) noexcept {
        return {lhs.x && rhs.x, lhs.y && rhs.y, lhs.z && rhs.z, lhs.w && rhs.w};
    }
    NOA_FHD constexpr Bool4 operator||(const Bool4& lhs, const Bool4& rhs) noexcept {
        return {lhs.x || rhs.x, lhs.y || rhs.y, lhs.z || rhs.z, lhs.w || rhs.w};
    }

    NOA_FHD constexpr bool any(const Bool4& v) noexcept {
        return v.x || v.y || v.z || v.w;
    }
    NOA_FHD constexpr bool all(const Bool4& v) noexcept {
        return v.x && v.y && v.z && v.w;
    }
}
