#include <random>
#include "noa/cpu/math/Random.h"

// TODO Add MKL support

namespace {
    static_assert(sizeof(long long) == 8, "64-bit machines only");

    // The "xoshiro256** 1.0" generator.
    // C++ port by Arthur O'Dwyer (2021).
    // Based on the C version by David Blackman and Sebastiano Vigna (2018),
    // https://prng.di.unimi.it/xoshiro256starstar.c
    struct xoshiro256ss {
        uint64_t s[4] {};

        static constexpr uint64_t splitmix64(uint64_t& x) {
            uint64_t z = (x += 0x9e3779b97f4a7c15uLL);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9uLL;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebuLL;
            return z ^ (z >> 31);
        }

        constexpr explicit xoshiro256ss() : xoshiro256ss(0) {}

        constexpr explicit xoshiro256ss(uint64_t seed) {
            s[0] = splitmix64(seed);
            s[1] = splitmix64(seed);
            s[2] = splitmix64(seed);
            s[3] = splitmix64(seed);
        }

        using result_type = uint64_t;
        static constexpr uint64_t min() { return 0; }
        static constexpr uint64_t max() { return uint64_t(-1); }

        static constexpr uint64_t rotl(uint64_t x, int k) {
            return (x << k) | (x >> (64 - k));
        }

        constexpr uint64_t operator()() {
            uint64_t result = rotl(s[1] * 5, 7) * 9;
            uint64_t t = s[1] << 17;
            s[2] ^= s[0];
            s[3] ^= s[1];
            s[1] ^= s[2];
            s[0] ^= s[3];
            s[2] ^= t;
            s[3] = rotl(s[3], 45);
            return result;
        }
    };

    using namespace ::noa;

    template<typename T>
    using supported_float = std::conditional_t<std::is_same_v<T, half_t>, float, T>;

    template<typename T>
    using uniform_distributor_t = std::conditional_t<std::is_integral_v<T>,
                                                     std::uniform_int_distribution<T>,
                                                     std::uniform_real_distribution<supported_float<T>>>;

    template<typename T>
    using normal_distributor_t = std::conditional_t<std::is_integral_v<T>,
                                                    std::normal_distribution<float>,
                                                    std::normal_distribution<supported_float<T>>>;

    template<typename T>
    using lognormal_distributor_t = std::conditional_t<std::is_integral_v<T>,
                                                       std::lognormal_distribution<float>,
                                                       std::lognormal_distribution<supported_float<T>>>;

    template<typename T>
    using poisson_distributor_t = std::conditional_t<std::is_integral_v<T>,
                                                     std::poisson_distribution<T>,
                                                     std::poisson_distribution<int64_t>>;

    template<typename T, typename distributor_t>
    void generate_(T* ptr, dim_t elements, distributor_t& distributor) {
        std::random_device seeder;
        xoshiro256ss generator(seeder());

        for (dim_t i = 0; i < elements; ++i)
            ptr[i] = static_cast<T>(distributor(generator));
    }

    template<typename T, typename distributor_t>
    void generate_(T* ptr, dim4_t strides, dim4_t shape, distributor_t& distributor) {
        std::random_device seeder;
        xoshiro256ss generator(seeder());

        const dim4_t order = indexing::order(strides, shape);
        strides = indexing::reorder(strides, order);
        shape = indexing::reorder(shape, order);

        for (dim_t i = 0; i < shape[0]; ++i)
            for (dim_t j = 0; j < shape[1]; ++j)
                for (dim_t k = 0; k < shape[2]; ++k)
                    for (dim_t l = 0; l < shape[3]; ++l)
                        ptr[indexing::at(i, j, k, l, strides)] = static_cast<T>(distributor(generator));
    }

    template<typename T, typename distributor_t>
    void generate_(Complex<T>* ptr, dim_t elements,
                   distributor_t& distributor_real, distributor_t& distributor_imag) {
        std::random_device seeder;
        xoshiro256ss generator(seeder());

        for (dim_t i = 0; i < elements; ++i)
            ptr[i] = static_cast<Complex<T>>(distributor_real(generator), distributor_imag(generator));
    }

    template<typename T, typename distributor_t>
    void generate_(Complex<T>* ptr, dim4_t strides, dim4_t shape,
                   distributor_t& distributor_real, distributor_t& distributor_imag) {
        std::random_device seeder;
        xoshiro256ss generator(seeder());

        const dim4_t order = indexing::order(strides, shape);
        strides = indexing::reorder(strides, order);
        shape = indexing::reorder(shape, order);

        for (dim_t i = 0; i < shape[0]; ++i)
            for (dim_t j = 0; j < shape[1]; ++j)
                for (dim_t k = 0; k < shape[2]; ++k)
                    for (dim_t l = 0; l < shape[3]; ++l)
                        ptr[indexing::at(i, j, k, l, strides)] =
                                static_cast<Complex<T>>(distributor_real(generator), distributor_imag(generator));
    }
}

namespace noa::cpu::math {
    template<typename T, typename U, typename>
    void randomize(noa::math::uniform_t, const shared_t<T[]>& output, dim_t elements,
                   U min, U max, Stream& stream) {
        NOA_ASSERT(output);
        if constexpr (noa::traits::is_complex_v<T>) {
            if constexpr (noa::traits::is_float_v<U>) {
                using real_t = noa::traits::value_type_t<T>;
                using supported_float = std::conditional_t<std::is_same_v<real_t, half_t>, float, real_t>;
                randomize(noa::math::uniform_t{}, std::reinterpret_pointer_cast<real_t[]>(output), elements * 2,
                          static_cast<supported_float>(min), static_cast<supported_float>(max), stream);
            } else {
                using real_t = noa::traits::value_type_t<T>;
                using distributor_t = uniform_distributor_t<real_t>;
                using result_t = typename distributor_t::result_type;
                stream.enqueue([=]() {
                    auto min_ = static_cast<result_t>(min.real);
                    auto max_ = static_cast<result_t>(max.real);
                    distributor_t distributor_real(min_, max_);
                    min_ = static_cast<result_t>(min.imag);
                    max_ = static_cast<result_t>(max.imag);
                    distributor_t distributor_imag(min_, max_);
                    generate_(output.get(), elements, distributor_real, distributor_imag);
                });
            }
        } else {
            using distributor_t = uniform_distributor_t<T>;
            using result_t = typename distributor_t::result_type;
            stream.enqueue([=]() {
                const auto min_ = static_cast<result_t>(min);
                const auto max_ = static_cast<result_t>(max);
                distributor_t distributor(min_, max_);
                generate_(output.get(), elements, distributor);
            });
        }
    }

    template<typename T, typename U, typename>
    void randomize(noa::math::normal_t, const shared_t<T[]>& output, dim_t elements,
                   U mean, U stddev, Stream& stream) {
        NOA_ASSERT(output);
        if constexpr (noa::traits::is_complex_v<T>) {
            if constexpr (noa::traits::is_float_v<U>) {
                using real_t = noa::traits::value_type_t<T>;
                using supported_float = std::conditional_t<std::is_same_v<real_t, half_t>, float, real_t>;
                randomize(noa::math::normal_t{}, std::reinterpret_pointer_cast<real_t[]>(output), elements * 2,
                          static_cast<supported_float>(mean), static_cast<supported_float>(stddev), stream);
            } else {
                using real_t = noa::traits::value_type_t<T>;
                using distributor_t = normal_distributor_t<real_t>;
                using result_t = typename distributor_t::result_type;
                stream.enqueue([=]() {
                    auto mean_ = static_cast<result_t>(mean.real);
                    auto stddev_ = static_cast<result_t>(stddev.real);
                    distributor_t distributor_real(mean_, stddev_);
                    mean_ = static_cast<result_t>(mean.imag);
                    stddev_ = static_cast<result_t>(stddev.imag);
                    distributor_t distributor_imag(mean_, stddev_);
                    generate_(output.get(), elements, distributor_real, distributor_imag);
                });
            }
        } else {
            using distributor_t = normal_distributor_t<T>;
            using result_t = typename distributor_t::result_type;
            stream.enqueue([=]() {
                const auto mean_ = static_cast<result_t>(mean);
                const auto stddev_ = static_cast<result_t>(stddev);
                distributor_t distributor(mean_, stddev_);
                generate_(output.get(), elements, distributor);
            });
        }
    }

    template<typename T, typename U, typename>
    void randomize(noa::math::log_normal_t, const shared_t<T[]>& output, dim_t elements,
                   U mean, U stddev, Stream& stream) {
        NOA_ASSERT(output);
        if constexpr (noa::traits::is_complex_v<T>) {
            if constexpr (noa::traits::is_float_v<U>) {
                using real_t = noa::traits::value_type_t<T>;
                using supported_float = std::conditional_t<std::is_same_v<real_t, half_t>, float, real_t>;
                randomize(noa::math::log_normal_t{}, std::reinterpret_pointer_cast<real_t[]>(output), elements * 2,
                          static_cast<supported_float>(mean), static_cast<supported_float>(stddev), stream);
            } else {
                using real_t = noa::traits::value_type_t<T>;
                using distributor_t = lognormal_distributor_t<real_t>;
                using result_t = typename distributor_t::result_type;
                stream.enqueue([=]() {
                    auto mean_ = static_cast<result_t>(mean.real);
                    auto stddev_ = static_cast<result_t>(stddev.real);
                    distributor_t distributor_real(mean_, stddev_);
                    mean_ = static_cast<result_t>(mean.imag);
                    stddev_ = static_cast<result_t>(stddev.imag);
                    distributor_t distributor_imag(mean_, stddev_);
                    generate_(output.get(), elements, distributor_real, distributor_imag);
                });
            }
        } else {
            using distributor_t = lognormal_distributor_t<T>;
            using result_t = typename distributor_t::result_type;
            stream.enqueue([=]() {
                const auto mean_ = static_cast<result_t>(mean);
                const auto stddev_ = static_cast<result_t>(stddev);
                distributor_t distributor(mean_, stddev_);
                generate_(output.get(), elements, distributor);
            });
        }
    }

    template<typename T, typename>
    void randomize(noa::math::poisson_t, const shared_t<T[]>& output, dim_t elements, float lambda, Stream& stream) {
        NOA_ASSERT(output);
        if constexpr (noa::traits::is_complex_v<T>) {
            using real_t = noa::traits::value_type_t<T>;
            randomize(noa::math::poisson_t{}, std::reinterpret_pointer_cast<real_t[]>(output), elements * 2,
                      lambda, stream);
        } else {
            using distributor_t = poisson_distributor_t<T>;
            stream.enqueue([=]() {
                distributor_t distributor(static_cast<double>(lambda));
                generate_(output.get(), elements, distributor);
            });
        }
    }

    template<typename T, typename U, typename>
    void randomize(noa::math::uniform_t, const shared_t<T[]>& output, dim4_t strides, dim4_t shape,
                   U min, U max, Stream& stream) {
        NOA_ASSERT(output && all(shape > 0));
        if (indexing::areContiguous(strides, shape))
            return randomize(noa::math::uniform_t{}, output, shape.elements(), min, max, stream);

        if constexpr (noa::traits::is_complex_v<T>) {
            if constexpr (noa::traits::is_float_v<U>) {
                using real_t = noa::traits::value_type_t<T>;
                using supported_float = std::conditional_t<std::is_same_v<real_t, half_t>, float, real_t>;
                const auto reinterpreted = indexing::Reinterpret<T>(shape, strides, output.get()).template as<real_t>();
                randomize(noa::math::uniform_t{}, std::reinterpret_pointer_cast<real_t[]>(output),
                          reinterpreted.strides, reinterpreted.shape,
                          static_cast<supported_float>(min), static_cast<supported_float>(max), stream);
            } else {
                using real_t = noa::traits::value_type_t<T>;
                using distributor_t = uniform_distributor_t<real_t>;
                using result_t = typename distributor_t::result_type;
                stream.enqueue([=]() {
                    auto min_ = static_cast<result_t>(min.real);
                    auto max_ = static_cast<result_t>(max.real);
                    distributor_t distributor_real(min_, max_);
                    min_ = static_cast<result_t>(min.imag);
                    max_ = static_cast<result_t>(max.imag);
                    distributor_t distributor_imag(min_, max_);
                    generate_(output.get(), strides, shape, distributor_real, distributor_imag);
                });
            }
        } else {
            using distributor_t = uniform_distributor_t<T>;
            using result_t = typename distributor_t::result_type;
            stream.enqueue([=]() {
                const auto min_ = static_cast<result_t>(min);
                const auto max_ = static_cast<result_t>(max);
                distributor_t distributor(min_, max_);
                generate_(output.get(), strides, shape, distributor);
            });
        }
    }

    template<typename T, typename U, typename>
    void randomize(noa::math::normal_t, const shared_t<T[]>& output, dim4_t strides, dim4_t shape,
                   U mean, U stddev, Stream& stream) {
        NOA_ASSERT(output && all(shape > 0));
        if (indexing::areContiguous(strides, shape))
            return randomize(noa::math::normal_t{}, output, shape.elements(), mean, stddev, stream);

        if constexpr (noa::traits::is_complex_v<T>) {
            if constexpr (noa::traits::is_float_v<U>) {
                using real_t = noa::traits::value_type_t<T>;
                using supported_float = std::conditional_t<std::is_same_v<real_t, half_t>, float, real_t>;
                const auto reinterpreted = indexing::Reinterpret<T>(shape, strides, output.get()).template as<real_t>();
                randomize(noa::math::normal_t{}, std::reinterpret_pointer_cast<real_t[]>(output),
                          reinterpreted.strides, reinterpreted.shape,
                          static_cast<supported_float>(mean), static_cast<supported_float>(stddev), stream);
            } else {
                using real_t = noa::traits::value_type_t<T>;
                using distributor_t = normal_distributor_t<real_t>;
                using result_t = typename distributor_t::result_type;
                stream.enqueue([=]() {
                    auto mean_ = static_cast<result_t>(mean.real);
                    auto stddev_ = static_cast<result_t>(stddev.real);
                    distributor_t distributor_real(mean_, stddev_);
                    mean_ = static_cast<result_t>(mean.imag);
                    stddev_ = static_cast<result_t>(stddev.imag);
                    distributor_t distributor_imag(mean_, stddev_);
                    generate_(output.get(), strides, shape, distributor_real, distributor_imag);
                });
            }
        } else {
            using distributor_t = normal_distributor_t<T>;
            using result_t = typename distributor_t::result_type;
            stream.enqueue([=]() {
                const auto mean_ = static_cast<result_t>(mean);
                const auto stddev_ = static_cast<result_t>(stddev);
                distributor_t distributor(mean_, stddev_);
                generate_(output.get(), strides, shape, distributor);
            });
        }
    }

    template<typename T, typename U, typename>
    void randomize(noa::math::log_normal_t, const shared_t<T[]>& output, dim4_t strides, dim4_t shape,
                   U mean, U stddev, Stream& stream) {
        NOA_ASSERT(output && all(shape > 0));
        if (indexing::areContiguous(strides, shape))
            return randomize(noa::math::log_normal_t{}, output, shape.elements(), mean, stddev, stream);

        if constexpr (noa::traits::is_complex_v<T>) {
            if constexpr (noa::traits::is_float_v<U>) {
                using real_t = noa::traits::value_type_t<T>;
                using supported_float = std::conditional_t<std::is_same_v<real_t, half_t>, float, real_t>;
                const auto reinterpreted = indexing::Reinterpret<T>(shape, strides, output.get()).template as<real_t>();
                randomize(noa::math::log_normal_t{}, std::reinterpret_pointer_cast<real_t[]>(output),
                          reinterpreted.strides, reinterpreted.shape,
                          static_cast<supported_float>(mean), static_cast<supported_float>(stddev), stream);
            } else {
                using real_t = noa::traits::value_type_t<T>;
                using distributor_t = lognormal_distributor_t<real_t>;
                using result_t = typename distributor_t::result_type;
                stream.enqueue([=]() {
                    auto mean_ = static_cast<result_t>(mean.real);
                    auto stddev_ = static_cast<result_t>(stddev.real);
                    distributor_t distributor_real(mean_, stddev_);
                    mean_ = static_cast<result_t>(mean.imag);
                    stddev_ = static_cast<result_t>(stddev.imag);
                    distributor_t distributor_imag(mean_, stddev_);
                    generate_(output.get(), strides, shape, distributor_real, distributor_imag);
                });
            }
        } else {
            using distributor_t = lognormal_distributor_t<T>;
            using result_t = typename distributor_t::result_type;
            stream.enqueue([=]() {
                const auto mean_ = static_cast<result_t>(mean);
                const auto stddev_ = static_cast<result_t>(stddev);
                distributor_t distributor(mean_, stddev_);
                generate_(output.get(), strides, shape, distributor);
            });
        }
    }

    template<typename T, typename>
    void randomize(noa::math::poisson_t, const shared_t<T[]>& output, dim4_t strides, dim4_t shape,
                   float lambda, Stream& stream) {
        NOA_ASSERT(output && all(shape > 0));
        if (indexing::areContiguous(strides, shape))
            return randomize(noa::math::poisson_t{}, output, shape.elements(), lambda, stream);

        if constexpr (noa::traits::is_complex_v<T>) {
            using real_t = noa::traits::value_type_t<T>;
            const auto reinterpreted = indexing::Reinterpret<T>(shape, strides, output.get()).template as<real_t>();
            randomize(noa::math::poisson_t{}, std::reinterpret_pointer_cast<real_t[]>(output),
                      reinterpreted.strides, reinterpreted.shape, lambda, stream);
        } else {
            using distributor_t = poisson_distributor_t<T>;
            stream.enqueue([=]() {
                distributor_t distributor(static_cast<double>(lambda));
                generate_(output.get(), strides, shape, distributor);
            });
        }
    }

    #define INSTANTIATE_RANDOM_(T, U)                                                                                   \
    template void randomize<T, U, void>(noa::math::uniform_t, const shared_t<T[]>&, dim4_t, dim4_t, U, U, Stream&);     \
    template void randomize<T, U, void>(noa::math::normal_t, const shared_t<T[]>&, dim4_t, dim4_t, U, U, Stream&);      \
    template void randomize<T, U, void>(noa::math::log_normal_t, const shared_t<T[]>&, dim4_t, dim4_t, U, U, Stream&);  \
    template void randomize<T, U, void>(noa::math::uniform_t, const shared_t<T[]>&, dim_t, U, U, Stream&);              \
    template void randomize<T, U, void>(noa::math::normal_t, const shared_t<T[]>&, dim_t, U, U, Stream&);               \
    template void randomize<T, U, void>(noa::math::log_normal_t, const shared_t<T[]>&, dim_t, U, U, Stream&)

    INSTANTIATE_RANDOM_(int16_t, int16_t);
    INSTANTIATE_RANDOM_(uint16_t, uint16_t);
    INSTANTIATE_RANDOM_(int32_t, int32_t);
    INSTANTIATE_RANDOM_(uint32_t, uint32_t);
    INSTANTIATE_RANDOM_(int64_t, int64_t);
    INSTANTIATE_RANDOM_(uint64_t, uint64_t);
    INSTANTIATE_RANDOM_(half_t, half_t);
    INSTANTIATE_RANDOM_(float, float);
    INSTANTIATE_RANDOM_(double, double);
    INSTANTIATE_RANDOM_(chalf_t, half_t);
    INSTANTIATE_RANDOM_(cfloat_t, float);
    INSTANTIATE_RANDOM_(cdouble_t, double);
    INSTANTIATE_RANDOM_(chalf_t, chalf_t);
    INSTANTIATE_RANDOM_(cfloat_t, cfloat_t);
    INSTANTIATE_RANDOM_(cdouble_t, cdouble_t);

    #define INSTANTIATE_RANDOM_POISSON_(T)                                                                  \
    template void randomize<T, void>(noa::math::poisson_t, const shared_t<T[]>&, dim_t, float, Stream&);    \
    template void randomize<T, void>(noa::math::poisson_t, const shared_t<T[]>&, dim4_t, dim4_t, float, Stream&)

    INSTANTIATE_RANDOM_POISSON_(half_t);
    INSTANTIATE_RANDOM_POISSON_(float);
    INSTANTIATE_RANDOM_POISSON_(double);

    #define INSTANTIATE_RANDOM_HALF_(T, U)                                                                              \
    template void randomize<T, U, void>(noa::math::uniform_t, const shared_t<T[]>&, dim4_t, dim4_t, U, U, Stream&);     \
    template void randomize<T, U, void>(noa::math::normal_t, const shared_t<T[]>&, dim4_t, dim4_t, U, U, Stream&);      \
    template void randomize<T, U, void>(noa::math::log_normal_t, const shared_t<T[]>&, dim4_t, dim4_t, U, U, Stream&);  \
    template void randomize<T, U, void>(noa::math::uniform_t, const shared_t<T[]>&, dim_t, U, U, Stream&);              \
    template void randomize<T, U, void>(noa::math::normal_t, const shared_t<T[]>&, dim_t, U, U, Stream&);               \
    template void randomize<T, U, void>(noa::math::log_normal_t, const shared_t<T[]>&, dim_t, U, U, Stream&)

    INSTANTIATE_RANDOM_HALF_(half_t, float);
    INSTANTIATE_RANDOM_HALF_(chalf_t, float);
    INSTANTIATE_RANDOM_HALF_(chalf_t, cfloat_t);
}
