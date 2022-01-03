#include "noa/gpu/cuda/fft/Transforms.h"
#include "noa/gpu/cuda/memory/PtrDevice.h"
#include "noa/gpu/cuda/memory/PtrDevicePadded.h"
#include "noa/gpu/cuda/memory/Copy.h"

#include <noa/cpu/fft/Transforms.h>
#include <noa/cpu/memory/PtrHost.h>

#include <catch2/catch.hpp>
#include "Helpers.h"

using namespace noa;

TEMPLATE_TEST_CASE("cuda::fft::r2c(), c2r()", "[noa][cuda][fft]", float, double) {
    using complex_t = Complex<TestType>;
    test::Randomizer<TestType> randomizer(-1, 1);
    test::Randomizer<complex_t> randomizer_complex(-1., 1.);

    uint ndim = GENERATE(1U, 2U, 3U);
    size3_t shape_real = test::getRandomShape(ndim);
    size3_t shape_complex = shapeFFT(shape_real);
    size_t elements_real = elements(shape_real);
    size_t elements_complex = elements(shape_complex);
    INFO(shape_real);

    double abs_epsilon;
    if constexpr (std::is_same_v<TestType, float>)
        abs_epsilon = 1e-3; // most are okay at 1e-5 but there are some outliers...
    else if constexpr (std::is_same_v<TestType, double>)
        abs_epsilon = 1e-9;

    cuda::Stream stream(cuda::Stream::CONCURRENT);
    cpu::Stream cpu_stream;

    AND_THEN("one time transform; out-of-place; R2C/C2R") {
        cpu::memory::PtrHost<TestType> h_real(elements_real);
        cpu::memory::PtrHost<complex_t> h_transform(elements_complex);
        cpu::memory::PtrHost<complex_t> h_transform_cuda(elements_complex);
        cuda::memory::PtrDevice<TestType> d_real(elements_real);
        cuda::memory::PtrDevice<complex_t> d_transform(elements_complex);

        test::randomize(h_real.get(), h_real.elements(), randomizer);
        cuda::memory::copy(h_real.get(), d_real.get(), h_real.size());

        // R2C
        cuda::fft::r2c(d_real.get(), d_transform.get(), shape_real, 1, stream);
        cuda::memory::copy(d_transform.get(), h_transform_cuda.get(), h_transform.size(), stream);
        cpu::fft::r2c(h_real.get(), h_transform.get(), shape_real, 1, cpu_stream);
        cuda::Stream::synchronize(stream);
        REQUIRE(test::Matcher(test::MATCH_ABS_SAFE,
                              h_transform.get(), h_transform_cuda.get(), h_transform.elements(), abs_epsilon));

        // Reset data
        test::randomize(h_transform.get(), h_transform.elements(), randomizer_complex);
        cuda::memory::copy(h_transform.get(), d_transform.get(), h_transform.size(), stream);

        cpu::memory::PtrHost<TestType> h_real_cuda(h_real.elements());
        test::memset(h_real.get(), h_real.elements(), 0);
        test::memset(h_real_cuda.get(), h_real.elements(), 0);

        // C2R
        cuda::fft::c2r(d_transform.get(), d_real.get(), shape_real, 1, stream);
        cuda::memory::copy(d_real.get(), h_real_cuda.get(), d_real.size(), stream);
        cpu::fft::c2r(h_transform.get(), h_real.get(), shape_real, 1, cpu_stream);
        cuda::Stream::synchronize(stream);

        REQUIRE(test::Matcher(test::MATCH_ABS_SAFE,
                              h_real.get(), h_real_cuda.get(), h_real_cuda.elements(), abs_epsilon));
    }

    AND_THEN("one time transform; out-of-place; R2C/C2R; padded memory") {
        cpu::memory::PtrHost<TestType> h_real(elements_real);
        cpu::memory::PtrHost<complex_t> h_transform(elements_complex);
        cuda::memory::PtrDevicePadded<TestType> d_real(shape_real);
        cuda::memory::PtrDevicePadded<complex_t> d_transform(shape_complex);
        cpu::memory::PtrHost<complex_t> h_transform_cuda(elements_complex);

        test::randomize(h_real.get(), h_real.elements(), randomizer);
        cuda::memory::copy(h_real.get(), shape_real.x, d_real.get(), d_real.pitch(), shape_real);

        // R2C
        cuda::fft::Plan<TestType> plan_r2c(cuda::fft::R2C, shape_real, 1, d_real.pitch(), d_transform.pitch(), stream);
        cuda::fft::r2c(d_real.get(), d_transform.get(), plan_r2c);
        cuda::memory::copy(d_transform.get(), d_transform.pitch(),
                           h_transform_cuda.get(), shape_complex.x, shape_complex, stream);
        cpu::fft::r2c(h_real.get(), h_transform.get(), shape_real, 1, cpu_stream);
        cuda::Stream::synchronize(stream);

        REQUIRE(test::Matcher(test::MATCH_ABS_SAFE,
                              h_transform.get(), h_transform_cuda.get(), h_transform.elements(), abs_epsilon));

        // Reset data
        test::randomize(h_transform.get(), h_transform.elements(), randomizer_complex);
        cuda::memory::copy(h_transform.get(), shape_complex.x,
                           d_transform.get(), d_transform.pitch(), shape_complex);

        cpu::memory::PtrHost<TestType> h_real_cuda(h_real.elements());
        test::memset(h_real.get(), h_real.elements(), 0);
        test::memset(h_real_cuda.get(), h_real.elements(), 0);

        // C2R
        cuda::fft::Plan<TestType> plan_c2r(cuda::fft::C2R, shape_real, 1, d_transform.pitch(), d_real.pitch(), stream);
        cuda::fft::c2r(d_transform.get(), d_real.get(), plan_c2r);
        cuda::memory::copy(d_real.get(), d_real.pitch(), h_real_cuda.get(), shape_real.x, shape_real, stream);
        cpu::fft::c2r(h_transform.get(), h_real.get(), shape_real, 1, cpu_stream);
        cuda::Stream::synchronize(stream);

        REQUIRE(test::Matcher(test::MATCH_ABS_SAFE,
                              h_real.get(), h_real_cuda.get(), h_real_cuda.elements(), abs_epsilon));
    }

    AND_THEN("one time transform; in-place; R2C/C2R") {
        // We tend to not pad the rows, we tend to ignore in-place transforms with FFTW (they are supported and
        // tested but one need to take care of the padding) since it makes everything more complicated...
        // In this case, just ignore the padding and compare to FFTW.
        cpu::memory::PtrHost<TestType> h_real(elements_real);
        cpu::memory::PtrHost<complex_t> h_transform(elements_complex);

        cuda::memory::PtrDevice<complex_t> d_input(elements_complex); // enough to contain the real and transform.
        auto* d_real = reinterpret_cast<TestType*>(d_input.get());
        auto* d_transform = d_input.get();
        size_t pitch_real = shape_real.x + ((shape_real.x % 2) ? 1 : 2);

        test::randomize(h_real.get(), h_real.elements(), randomizer);
        cuda::memory::copy(h_real.get(), shape_real.x, d_real, pitch_real, shape_real);

        // R2C
        cpu::fft::r2c(h_real.get(), h_transform.get(), shape_real, 1, cpu_stream);
        cuda::fft::r2c(d_real, d_transform, shape_real, 1, stream);
        cuda::Stream::synchronize(stream);

        cpu::memory::PtrHost<complex_t> h_transform_cuda(elements_complex);
        cuda::memory::copy(d_transform, h_transform_cuda.get(), h_transform_cuda.size());

        REQUIRE(test::Matcher(test::MATCH_ABS_SAFE,
                              h_transform.get(), h_transform_cuda.get(), h_transform.elements(), abs_epsilon));

        // Reset data
        test::randomize(h_transform.get(), elements_complex, randomizer_complex);
        cuda::memory::copy(h_transform.get(), d_transform, h_transform.size());

        cpu::memory::PtrHost<TestType> h_real_cuda(elements_real);
        test::memset(h_real.get(), elements_real, 0);
        test::memset(h_real_cuda.get(), h_real_cuda.elements(), 0);

        // C2R
        cpu::fft::c2r(h_transform.get(), h_real.get(), shape_real, 1, cpu_stream);
        cuda::fft::c2r(d_transform, d_real, shape_real, 1, stream);
        cuda::memory::copy(d_real, pitch_real, h_real_cuda.get(), shape_real.x, shape_real, stream);
        cuda::Stream::synchronize(stream);

        REQUIRE(test::Matcher(test::MATCH_ABS_SAFE,
                              h_real.get(), h_real_cuda.get(), h_real_cuda.elements(), abs_epsilon));
    }
}

TEMPLATE_TEST_CASE("cuda::fft::c2c()", "[noa][cuda][fft]", cfloat_t, cdouble_t) {
    using real_t = noa::traits::value_type_t<TestType>;
    test::Randomizer<TestType> randomizer(-1., 1.);

    uint ndim = GENERATE(1U, 2U, 3U);
    size3_t shape = test::getRandomShape(ndim);
    size_t elements = noa::elements(shape);
    INFO(shape);

    double abs_epsilon;
    if constexpr (std::is_same_v<TestType, cfloat_t>)
        abs_epsilon = 1e-3;
    else if constexpr (std::is_same_v<TestType, cdouble_t>)
        abs_epsilon = 1e-9;

    cuda::Stream stream(cuda::Stream::CONCURRENT);
    cpu::Stream cpu_stream;

    AND_THEN("one time transform; out-of-place; C2C") {
        cpu::memory::PtrHost<TestType> h_input(elements);
        cpu::memory::PtrHost<TestType> h_output(elements);
        cuda::memory::PtrDevice<TestType> d_input(elements);
        cuda::memory::PtrDevice<TestType> d_output(elements);

        test::randomize(h_input.get(), elements, randomizer);
        cuda::memory::copy(h_input.get(), d_input.get(), h_input.size());

        // Forward
        cpu::fft::c2c(h_input.get(), h_output.get(), shape, 1, cpu::fft::FORWARD, cpu_stream);
        cuda::fft::c2c(d_input.get(), d_output.get(), shape, 1, cuda::fft::FORWARD, stream);

        cpu::memory::PtrHost<TestType> h_output_cuda(elements);
        cuda::memory::copy(d_output.get(), h_output_cuda.get(), d_output.size(), stream);
        cuda::Stream::synchronize(stream);

        REQUIRE(test::Matcher(test::MATCH_ABS_SAFE, h_output.get(), h_output_cuda.get(), elements, abs_epsilon));

        // Reset data
        test::randomize(h_input.get(), elements, randomizer);
        cuda::memory::copy(h_input.get(), d_input.get(), h_input.size());

        // Backward
        cpu::fft::c2c(h_input.get(), h_output.get(), shape, 1, cpu::fft::BACKWARD, cpu_stream);
        cuda::fft::c2c(d_input.get(), d_output.get(), shape, 1, cuda::fft::BACKWARD, stream);
        cuda::memory::copy(d_output.get(), h_output_cuda.get(), d_output.size(), stream);
        cuda::Stream::synchronize(stream);

        REQUIRE(test::Matcher(test::MATCH_ABS_SAFE, h_output.get(), h_output_cuda.get(), elements, abs_epsilon));
    }

    AND_THEN("one time transform; out-of-place; C2C; padded memory") {
        cpu::memory::PtrHost<TestType> h_input(elements);
        cpu::memory::PtrHost<TestType> h_output(elements);
        cpu::memory::PtrHost<TestType> h_output_cuda(elements);
        cuda::memory::PtrDevicePadded<TestType> d_input(shape);
        cuda::memory::PtrDevicePadded<TestType> d_output(shape);

        test::randomize(h_input.get(), elements, randomizer);
        cuda::memory::copy(h_input.get(), shape.x, d_input.get(), d_input.pitch(), shape);

        // Forward
        cuda::fft::Plan<real_t> plan_c2c(cuda::fft::C2C, shape, 1, d_input.pitch(), d_output.pitch(), stream);
        cuda::fft::c2c(d_input.get(), d_output.get(), plan_c2c, cuda::fft::FORWARD);
        cuda::memory::copy(d_output.get(), d_output.pitch(), h_output_cuda.get(), shape.x, shape, stream);
        cpu::fft::c2c(h_input.get(), h_output.get(), shape, 1, cpu::fft::FORWARD, cpu_stream);
        cuda::Stream::synchronize(stream);

        REQUIRE(test::Matcher(test::MATCH_ABS_SAFE, h_output.get(), h_output_cuda.get(), elements, abs_epsilon));

        // Reset data
        test::randomize(h_input.get(), elements, randomizer);
        cuda::memory::copy(h_input.get(), shape.x, d_input.get(), d_input.pitch(), shape, stream);

        // Backward
        // We can reuse the plan since both arrays have the same shape and therefore same pitch.
        cuda::fft::c2c(d_input.get(), d_output.get(), plan_c2c, cuda::fft::BACKWARD);
        cuda::memory::copy(d_output.get(), d_output.pitch(), h_output_cuda.get(), shape.x, shape, stream);
        cpu::fft::c2c(h_input.get(), h_output.get(), shape, 1, cpu::fft::BACKWARD, cpu_stream);
        cuda::Stream::synchronize(stream);

        REQUIRE(test::Matcher(test::MATCH_ABS_SAFE, h_output.get(), h_output_cuda.get(), elements, abs_epsilon));
    }
}
