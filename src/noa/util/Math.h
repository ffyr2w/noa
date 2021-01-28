/**
 * @file Math.h
 * @brief Various "math related functions".
 * @author Thomas - ffyr2w
 * @date 20 Jul 2020
 */
#pragma once

#include <math.h> // for CUDA, as opposed to cmath
#include <cstdint>
#include <limits>

#include "noa/Define.h"
#include "noa/util/traits/BaseTypes.h"

#define ULP 2
#define EPSILON 1e-6f

/*
 * math.h: The CUDA math library supports all overloaded versions required by the C++ standard.
 *         It includes the host’s math.h header file and the corresponding device code.
 */

namespace Noa::Math {
    /* --- Trigonometric functions --- */

    /**  Returns the cosine of an angle of x radians. */
    NOA_DH inline double cos(double x) { return ::cos(x); }

    /**  Returns the sine of an angle of x radians. */
    NOA_DH inline double sin(double x) { return ::sin(x); }

    /**  Returns the tangent of an angle of x radians. */
    NOA_DH inline double tan(double x) { return ::tan(x); }

    /** Returns the principal value of the arc cos of x, in radians. */
    NOA_DH inline double acos(double x) { return ::acos(x); }

    /** Returns the principal value of the arc sine of x, in radians. */
    NOA_DH inline double asin(double x) { return ::asin(x); }

    /** Returns the principal value of the arc tangent of x, in radians. */
    NOA_DH inline double atan(double x) { return ::atan(x); }

    /** Returns the principal value of the arc tangent of y/x, in radians. */
    NOA_DH inline double atan2(double y, double x) { return ::atan2(y, x); }

    NOA_DH inline float cos(float x) { return ::cosf(x); }
    NOA_DH inline float sin(float x) { return ::sinf(x); }
    NOA_DH inline float tan(float x) { return ::tanf(x); }
    NOA_DH inline float acos(float x) { return ::acosf(x); }
    NOA_DH inline float asin(float x) { return ::asinf(x); }
    NOA_DH inline float atan(float x) { return ::atanf(x); }
    NOA_DH inline float atan2(float y, float x) { return ::atan2f(y, x); }

    /* --- Hyperbolic functions --- */

    /**  Returns the hyperbolic cosine of x. */
    NOA_DH inline double cosh(double x) { return ::cosh(x); }

    /**  Returns the hyperbolic sine of x. */
    NOA_DH inline double sinh(double x) { return ::sinh(x); }

    /**  Returns the hyperbolic tangent of x. */
    NOA_DH inline double tanh(double x) { return ::tanh(x); }

    /** Returns the non-negative area hyperbolic cosine of x. */
    NOA_DH inline double acosh(double x) { return ::acosh(x); }

    /** Returns the area hyperbolic sine of x. */
    NOA_DH inline double asinh(double x) { return ::asinh(x); }

    /** Returns the area hyperbolic tangent of x. */
    NOA_DH inline double atanh(double x) { return ::atanh(x); }

    NOA_DH inline float cosh(float x) { return ::coshf(x); }
    NOA_DH inline float sinh(float x) { return ::sinhf(x); }
    NOA_DH inline float tanh(float x) { return ::tanhf(x); }
    NOA_DH inline float acosh(float x) { return ::acoshf(x); }
    NOA_DH inline float asinh(float x) { return ::asinhf(x); }
    NOA_DH inline float atanh(float x) { return ::atanhf(x); }

    /* --- Exponential and logarithmic functions --- */

    /** Returns the exponential of @a x. */
    NOA_DH inline double exp(double x) { return ::exp(x); }
    NOA_DH inline float exp(float x) { return ::expf(x); }

    /** Returns the natural logarithm of @a x. */
    NOA_DH inline double log(double x) { return ::log(x); }
    NOA_DH inline float log(float x) { return ::logf(x); }

    /** Returns the base 10 logarithm of @a x. */
    NOA_DH inline double log10(double x) { return ::log10(x); }
    NOA_DH inline float log10(float x) { return ::log10f(x); }

    /** Returns the natural logarithm of one plus @a x. */
    NOA_DH inline double log1p(double x) { return ::log1p(x); }
    NOA_DH inline float log1p(float x) { return ::log1pf(x); }

    /* --- Power functions --- */

    /** Returns the hypotenuse of a right-angled triangle whose legs are @a x and @a y. */
    NOA_DH inline double hypot(double x, double y) { return ::hypot(x, y); }
    NOA_DH inline float hypot(float x, float y) { return ::hypotf(x, y); }

    /**  Returns @a base raised to the power @a exponent. */
    NOA_DH inline double pow(double base, double exponent) { return ::pow(base, exponent); }
    NOA_DH inline float pow(float base, float exponent) { return ::powf(base, exponent); }

    /** Returns the square root of @a x. */
    NOA_DH inline double sqrt(double x) { return ::sqrt(x); }
    NOA_DH inline float sqrt(float v) { return ::sqrtf(v); }

    /** Returns 1. / sqrt(@a x). */
    NOA_DH inline double rsqrt(double v) {
#ifdef __CUDA_ARCH__
        return ::rsqrt(v); // device code trajectory steered by nvcc
#else
        return 1. / ::sqrt(v);
#endif
    }

    NOA_DH inline float rsqrt(float v) {
#ifdef __CUDA_ARCH__
        return ::rsqrtf(v); // device code trajectory steered by nvcc
#else
        return 1.f / ::sqrtf(v);
#endif
    }

    /* --- Rounding and remainder functions --- */

    /** Rounds @a v to nearest integral value. */
    NOA_DH inline double round(double v) { return ::round(v); }
    NOA_DH inline float round(float v) { return ::roundf(v); }

    /** Rounds @a v to integral value. Should be preferred to round a double to an integer. */
    NOA_DH inline double rint(double v) { return ::rint(v); }
    NOA_DH inline float rint(float v) { return ::rintf(v); }

    /** Rounds up @a v. */
    NOA_DH inline double ceil(double v) { return ::ceil(v); }
    NOA_DH inline float ceil(float v) { return ::ceilf(v); }

    /** Rounds down @a v. */
    NOA_DH inline double floor(double v) { return ::floor(v); }
    NOA_DH inline float floor(float v) { return ::floorf(v); }

    /** Truncates @a v. */
    NOA_DH inline double trunc(double v) { return ::trunc(v); }
    NOA_DH inline float trunc(float v) { return ::truncf(v); }

    /* --- Floating-point manipulation functions --- */

    /** Returns a value with the magnitude of x and the sign of y. */
    NOA_DH constexpr inline double copysign(double x, double y) { return ::copysign(x, y); }
    NOA_DH constexpr inline float copysign(float x, float y) { return ::copysign(x, y); }

    /* --- Classification --- */

    /** Returns whether x is a NaN (Not-A-Number) value. */
    NOA_DH inline bool isNaN(double v) { return ::isnan(v); }
    NOA_DH inline bool isNaN(float v) { return ::isnan(v); }

    /** Returns whether x is an infinity value (either positive infinity or negative infinity). */
    NOA_DH inline bool isInf(double v) { return ::isinf(v); }
    NOA_DH inline bool isInf(float v) { return ::isinf(v); }

    /** Returns whether x is a finite value (i.e. neither inf nor NaN). */
    NOA_DH inline bool isFinite(double v) { return ::isfinite(v); }
    NOA_DH inline bool isFinite(float v) { return ::isfinite(v); }

    /** Returns whether x is a normal value (i.e. neither inf, NaN, zero or subnormal. */
    NOA_DH inline bool isNormal(double v) { return ::isnormal(v); }
    NOA_DH inline bool isNormal(float v) { return ::isnormal(v); }

    /** Returns whether the sign of x is negative. Can be also applied to inf, NaNs and 0s (unsigned is positive). */
    NOA_DH inline bool signbit(double v) { return ::signbit(v); }
    NOA_DH inline bool signbit(float v) { return ::signbit(v); }

    /* --- Other functions --- */

    /** Returns the absolute value of @a v. */
    NOA_DH inline double abs(double v) { return ::abs(v); }
    NOA_DH inline float abs(float v) { return ::abs(v); }
    NOA_DH inline int abs(int v) { return ::abs(v); }
    NOA_DH inline long abs(long v) { return ::abs(v); }
    NOA_DH inline long abs(long long v) { return ::abs(v); }
}

namespace Noa::Math {
    /**
     * Whether or not two floating-points are "significantly" equal.
     * @details For the relative epsilon, the machine epsilon has to be scaled to the magnitude of
     *          the values used and multiplied by the desired precision in ULPs. The magnitude is
     *          often set as max(abs(x), abs(y)), but this function is setting the magnitude as
     *          abs(x+y), which is basically equivalent and is should be more efficient. Relative
     *          epsilons and Unit in the Last Place (ULPs) comparisons are usually meaningless for
     *          close-to-zero numbers, hence the absolute comparison with @a epsilon, acting as
     *          a safety net.
     * @note    If one or both values are NaN and|or +/-Inf, returns false.
     */
    template<uint32_t ulp = ULP, typename T, typename = std::enable_if_t<Noa::Traits::is_float_v<T>>>
    constexpr bool isEqual(T x, T y, T epsilon = EPSILON) {
        const auto diff = std::abs(x - y);
        if (!std::isfinite(diff))
            return false;

        return diff <= epsilon ||
               diff <= (std::abs(x + y) * std::numeric_limits<T>::epsilon() * ulp);
    }

    /**
     * Whether or not @a x is less or "significantly" equal than @a y.
     * @note    If one or both values are NaN and|or +/-Inf, returns false.
     */
    template<uint32_t ulp = ULP, typename T, typename = std::enable_if_t<Noa::Traits::is_float_v<T>>>
    constexpr bool isLessOrEqual(T x, T y, T epsilon = EPSILON) noexcept {
        const auto diff = x - y;
        if (!std::isfinite(diff))
            return false;

        return diff <= epsilon ||
               diff <= (std::abs(x + y) * std::numeric_limits<T>::epsilon() * ulp);
    }

    /**
     * Whether or not @a x is greater or "significantly" equal than @a y.
     * @note    If one or both values are NaN and|or +/-Inf, returns false.
     */
    template<uint32_t ulp = ULP, typename T, typename = std::enable_if_t<Noa::Traits::is_float_v<T>>>
    constexpr bool isGreaterOrEqual(T x, T y, T epsilon = EPSILON) noexcept {
        const auto diff = y - x;
        if (!std::isfinite(diff))
            return false;

        return diff <= epsilon ||
               diff <= (std::abs(x + y) * std::numeric_limits<T>::epsilon() * ulp);
    }

    /**
     * Whether or not @a x is "significantly" withing @a min and @a max.
     * @note    If one or all values are NaN and|or +/-Inf, returns false.
     */
    template<uint32_t ulp = ULP, typename T, typename = std::enable_if_t<Noa::Traits::is_float_v<T>>>
    constexpr inline bool isWithin(T x, T min, T max, T epsilon = EPSILON) noexcept {
        return isGreaterOrEqual<ulp>(x, min, epsilon) && isLessOrEqual<ulp>(x, max, epsilon);
    }

    /**
     * Whether or not @a x is "significantly" less than @a y.
     * @note    If one or both values are NaN and|or +/-Inf, returns false.
     */
    template<uint32_t ulp = ULP, typename T, typename = std::enable_if_t<Noa::Traits::is_float_v<T>>>
    constexpr bool isLess(T x, T y, T epsilon = EPSILON) noexcept {
        const auto diff = y - x;
        if (!std::isfinite(diff))
            return false;

        return diff > epsilon ||
               diff > (std::abs(x + y) * std::numeric_limits<T>::epsilon() * ulp);
    }

    /**
     * Whether or not @a x is "significantly" greater than @a y.
     * @note    If one or both values are NaN and|or +/-Inf, returns false.
     */
    template<uint32_t ulp = ULP, typename T, typename = std::enable_if_t<Noa::Traits::is_float_v<T>>>
    constexpr bool isGreater(T x, T y, T epsilon = EPSILON) noexcept {
        const auto diff = x - y;
        if (!std::isfinite(diff))
            return false;

        return diff > epsilon ||
               diff > (std::abs(x + y) * std::numeric_limits<T>::epsilon() * ulp);
    }
}

#undef ULP
#undef EPSILON
