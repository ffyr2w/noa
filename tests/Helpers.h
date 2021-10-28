#pragma once

#include <noa/common/Types.h>
#include <noa/common/Math.h>
#include <noa/common/traits/BaseTypes.h>

#include <random>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <catch2/catch.hpp>

namespace test {
    extern noa::path_t PATH_TEST_DATA; // defined at runtime by the main.
}

#define REQUIRE_FOR_ALL(range, predicate) for (auto& e: range) REQUIRE((predicate(e)))

namespace test {
    template<typename T, size_t N>
    inline std::vector<T> toVector(const std::array<T, N>& array) {
        return std::vector<T>(array.cbegin(), array.cend());
    }

    inline bool almostEquals(float actual, float expected, uint64_t maxUlpDiff = 2) {
        static_assert(sizeof(float) == sizeof(int32_t));

        if (std::isnan(actual) || std::isnan(expected))
            return false;

        auto convert = [](float value) {
            int32_t i;
            std::memcpy(&i, &value, sizeof(value));
            return i;
        };

        auto lc = convert(actual);
        auto rc = convert(expected);

        if ((lc < 0) != (rc < 0)) {
            // Potentially we can have +0 and -0
            return actual == expected;
        }

        auto ulpDiff = std::abs(lc - rc);
        return static_cast<uint64_t>(ulpDiff) <= maxUlpDiff;
    }

    inline bool almostEquals(double actual, double expected, uint64_t maxUlpDiff = 2) {
        static_assert(sizeof(double) == sizeof(int64_t));

        if (std::isnan(actual) || std::isnan(expected))
            return false;

        auto convert = [](double value) {
            int64_t i;
            std::memcpy(&i, &value, sizeof(value));
            return i;
        };

        auto lc = convert(actual);
        auto rc = convert(expected);

        if ((lc < 0) != (rc < 0))
            return actual == expected;

        auto ulpDiff = std::abs(lc - rc);
        return static_cast<uint64_t>(ulpDiff) <= maxUlpDiff;
    }
}

// RANDOM:
namespace test {
    template<typename T>
    class RealRandomizer {
    private:
        std::random_device rand_dev;
        std::mt19937 generator;
        std::uniform_real_distribution<T> distribution;
    public:
        template<typename U>
        RealRandomizer(U range_from, U range_to)
                : generator(rand_dev()), distribution(static_cast<T>(range_from), static_cast<T>(range_to)) {}
        inline T get() { return distribution(generator); }
    };

    template<>
    class RealRandomizer<noa::cfloat_t> {
    private:
        std::random_device rand_dev;
        std::mt19937 generator;
        std::uniform_real_distribution<float> distribution;
    public:
        RealRandomizer(float from, float to) : generator(rand_dev()), distribution(from, to) {}
        RealRandomizer(double from, double to)
                : generator(rand_dev()), distribution(static_cast<float>(from), static_cast<float>(to)) {}
        inline noa::cfloat_t get() { return noa::cfloat_t(distribution(generator), distribution(generator)); }
    };

    template<>
    class RealRandomizer<noa::cdouble_t> {
    private:
        std::random_device rand_dev{};
        std::mt19937 generator;
        std::uniform_real_distribution<double> distribution;
    public:
        RealRandomizer(double from, double to) : generator(rand_dev()), distribution(from, to) {}
        RealRandomizer(float from, float to)
                : generator(rand_dev()), distribution(static_cast<double>(from), static_cast<double>(to)) {}
        inline noa::cdouble_t get() { return noa::cdouble_t(distribution(generator), distribution(generator)); }
    };

    template<typename T>
    class IntRandomizer {
    private:
        std::random_device rand_dev;
        std::mt19937 generator;
        std::uniform_int_distribution<T> distribution;
    public:
        template<typename U>
        IntRandomizer(U range_from, U range_to)
                : generator(rand_dev()), distribution(static_cast<T>(range_from), static_cast<T>(range_to)) {}
        inline T get() { return distribution(generator); }
    };

    // More flexible
    template<typename T>
    struct MyIntRandomizer {
        using type = IntRandomizer<T>;
    };
    template<typename T>
    struct MyRealRandomizer {
        using type = RealRandomizer<T>;
    };
    template<typename T>
    using Randomizer =
    typename std::conditional_t<noa::traits::is_int_v<T>, MyIntRandomizer<T>, MyRealRandomizer<T>>::type;

    inline int pseudoRandom(int range_from, int range_to) {
        int out = range_from + std::rand() / (RAND_MAX / (range_to - range_from + 1) + 1);
        return out;
    }

    inline noa::size3_t getRandomShape(uint ndim) {
        if (ndim == 2) {
            test::IntRandomizer<size_t> randomizer(32, 512);
            return noa::size3_t{randomizer.get(), randomizer.get(), 1};
        } else if (ndim == 3) {
            test::IntRandomizer<size_t> randomizer(32, 128);
            return noa::size3_t{randomizer.get(), randomizer.get(), randomizer.get()};
        } else {
            test::IntRandomizer<size_t> randomizer(32, 1024);
            return noa::size3_t{randomizer.get(), 1, 1};
        }
    }

    inline noa::size3_t getRandomShape(uint ndim, bool even) {
        noa::size3_t shape = getRandomShape(ndim);
        if (even) {
            shape.x += shape.x % 2;
            if (ndim >= 2)
                shape.y += shape.y % 2;
            if (ndim == 3)
                shape.z += shape.z % 2;
        } else {
            shape.x += !(shape.x % 2);
            if (ndim >= 2)
                shape.y += !(shape.y % 2);
            if (ndim == 3)
                shape.z += !(shape.z % 2);
        }
        return shape;
    }
}

// INITIALIZE DATA:
namespace test {
    template<typename T, typename U>
    inline void initDataRandom(T* data, size_t elements, test::IntRandomizer<U>& randomizer) {
        if constexpr (std::is_same_v<T, noa::cfloat_t>) {
            initDataRandom(reinterpret_cast<float*>(data), elements * 2, randomizer);
            return;
        } else if constexpr (std::is_same_v<T, noa::cdouble_t>) {
            initDataRandom(reinterpret_cast<double*>(data), elements * 2, randomizer);
            return;
        } else {
            for (size_t idx{0}; idx < elements; ++idx)
                data[idx] = static_cast<T>(randomizer.get());
        }
    }

    template<typename T>
    inline void initDataRandom(T* data, size_t elements, test::RealRandomizer<T>& randomizer) {
        for (size_t idx{0}; idx < elements; ++idx)
            data[idx] = randomizer.get();
    }

    template<typename T>
    inline void initDataZero(T* data, size_t elements) {
        for (size_t idx{0}; idx < elements; ++idx)
            data[idx] = 0;
    }
}

// COMPUTE DIFFERENCES:
namespace test {
    template<typename T>
    inline T getDifference(const T* in, const T* out, size_t elements) {
        if constexpr (std::is_same_v<T, noa::cfloat_t> || std::is_same_v<T, noa::cdouble_t>) {
            T diff{0}, tmp;
            for (size_t idx{0}; idx < elements; ++idx) {
                tmp = out[idx] - in[idx];
                diff += T{tmp.real > 0 ? tmp.real : -tmp.real,
                          tmp.imag > 0 ? tmp.imag : -tmp.imag};
            }
            return diff;
        } else if constexpr (std::is_integral_v<T>) {
            int64_t diff{0};
            for (size_t idx{0}; idx < elements; ++idx)
                diff += std::abs(static_cast<int64_t>(out[idx] - in[idx]));
            return noa::clamp_cast<T>(diff);
        } else {
            T diff{0}, tmp;
            for (size_t idx{0}; idx < elements; ++idx) {
                tmp = out[idx] - in[idx];
                diff += tmp > 0 ? tmp : -tmp;
            }
            return diff;
        }
    }

    template<typename T>
    inline T getAverageDifference(const T* in, const T* out, size_t elements) {
        T diff = getDifference(in, out, elements);
        if constexpr (std::is_same_v<T, noa::cfloat_t>) {
            return diff / static_cast<float>(elements);
        } else if constexpr (std::is_same_v<T, noa::cdouble_t>) {
            return diff / static_cast<double>(elements);
        } else {
            return diff / static_cast<T>(elements);
        }
    }

    template<typename T, typename U>
    inline void normalize(T* array, size_t size, U scale) {
        for (size_t idx{0}; idx < size; ++idx) {
            array[idx] *= scale;
        }
    }

    // The differences are normalized by the magnitude of the values being compared. This is important since depending
    // on the magnitude of the floating-point being compared, the expected error will vary. This is exactly what
    // isWithinRel (or noa::math::isEqual) does but reduces the array to one value so that it can then be asserted by
    // REQUIRE isWithinAbs.
    // Use this version, as opposed to getDifference, when the values are expected to have a large magnitude.
    template<typename T>
    inline T getNormalizedDifference(const T* in, const T* out, size_t elements) {
        if constexpr (std::is_same_v<T, noa::cfloat_t> || std::is_same_v<T, noa::cdouble_t>) {
            using real_t = noa::traits::value_type_t<T>;
            return getNormalizedDifference(reinterpret_cast<const real_t*>(in),
                                           reinterpret_cast<const real_t*>(out),
                                           elements * 2);
        } else {
            T diff{0}, tmp, mag;
            for (size_t idx{0}; idx < elements; ++idx) {
                mag = noa::math::max(noa::math::abs(out[idx]), noa::math::abs(in[idx]));
                tmp = (out[idx] - in[idx]);
                if (mag != 0)
                    tmp /= mag;
                diff += tmp > 0 ? tmp : -tmp;
            }
            return diff;
        }
    }

    template<typename T>
    inline T getAverageNormalizedDifference(const T* in, const T* out, size_t elements) {
        T diff = getNormalizedDifference(in, out, elements);
        if constexpr (std::is_same_v<T, noa::cfloat_t>) {
            return diff / static_cast<float>(elements);
        } else if constexpr (std::is_same_v<T, noa::cdouble_t>) {
            return diff / static_cast<double>(elements);
        } else {
            return diff / static_cast<T>(elements);
        }
    }
}

// MATCHERS:
namespace test {
    template<typename T, typename U>
    class WithinAbs : public Catch::MatcherBase<T> {
    private:
        T m_expected;
        U m_epsilon;
    public:
        WithinAbs(T expected, U epsilon) : m_expected(expected), m_epsilon(epsilon) {}

        bool match(const T& value) const override {
            if constexpr (std::is_same_v<T, noa::cfloat_t>) {
                return noa::math::abs(value.real - m_expected.real) <= static_cast<float>(m_epsilon) &&
                       noa::math::abs(value.imag - m_expected.imag) <= static_cast<float>(m_epsilon);
            } else if constexpr (std::is_same_v<T, noa::cdouble_t>) {
                return noa::math::abs(value.real - m_expected.real) <= static_cast<double>(m_epsilon) &&
                       noa::math::abs(value.imag - m_expected.imag) <= static_cast<double>(m_epsilon);
            } else if constexpr (std::is_integral_v<T>) {
                return value - m_expected == 0;
            } else {
                return noa::math::abs(value - m_expected) <= static_cast<T>(m_epsilon);
            }
        }

        std::string describe() const override {
            std::ostringstream ss;
            if constexpr (std::is_integral_v<T>)
                ss << "is equal to " << m_expected;
            else
                ss << "is equal to " << m_expected << " +/- abs epsilon of " << m_epsilon;
            return ss.str();
        }
    };

    // Whether or not the tested value is equal to \a expected_value +/- \a epsilon.
    // \note For complex types, the same epsilon is applied to the real and imaginary part.
    // \note For integral types, \a epsilon is ignored.
    template<typename T, typename U, typename = std::enable_if_t<std::is_floating_point_v<U>>>
    inline WithinAbs<T, U> isWithinAbs(T expected_value, U epsilon) {
        return WithinAbs(expected_value, epsilon);
    }

    template<typename T, typename U>
    class WithinRel : public Catch::MatcherBase<T> {
    private:
        using real_t = noa::traits::value_type_t<T>;
        T m_expected;
        real_t m_epsilon;

        static bool isWithin_(real_t expected, real_t result, real_t epsilon) {
            auto margin = epsilon * noa::math::max(noa::math::abs(result), noa::math::abs(expected));
            if (std::isinf(margin))
                margin = 0;
            return (result + margin >= expected) && (expected + margin >= result); // abs(a-b) <= epsilon
        }

    public:
        WithinRel(T expected, U epsilon) : m_expected(expected), m_epsilon(static_cast<real_t>(epsilon)) {}

        bool match(const T& value) const override {
            if constexpr (noa::traits::is_complex_v<T>)
                return isWithin_(m_expected.real, value.real, m_epsilon) &&
                       isWithin_(m_expected.imag, value.imag, m_epsilon);
            else
                return isWithin_(m_expected, value, m_epsilon);
        }

        std::string describe() const override {
            std::ostringstream ss;
            ss << "and " << m_expected << " are within " << m_epsilon * 100 << "% of each other";
            return ss.str();
        }
    };

    // Whether or not the tested value and \a expected_value are within \a epsilon % of each other.
    // \note For complex types, the same epsilon is applied to the real and imaginary part.
    // \warning For close to zero or zeros, it might be necessary to have an absolute check since the epsilon is
    //          scaled by the value, resulting in an extremely small epsilon...
    template<typename T, typename U = noa::traits::value_type_t<T>>
    inline WithinRel<T, U> isWithinRel(T expected_value,
                                       U epsilon = noa::math::Limits<noa::traits::value_type_t<T>>::epsilon() * 100) {
        static_assert(std::is_floating_point_v<U> && (noa::traits::is_float_v<T> || noa::traits::is_complex_v<T>));
        return WithinRel(expected_value, epsilon);
    }
}
