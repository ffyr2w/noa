#include <noa/common/io/ImageFile.h>
#include <noa/common/transform/Euler.h>
#include <noa/common/transform/Geometry.h>

#include <noa/cpu/math/Ewise.h>
#include <noa/cpu/memory/PtrHost.h>
#include <noa/cpu/transform/fft/Apply.h>
#include <noa/cpu/transform/fft/Shift.h>
#include <noa/cpu/fft/Transforms.h>
#include <noa/cpu/fft/Remap.h>

#include "Assets.h"
#include "Helpers.h"
#include <catch2/catch.hpp>

using namespace ::noa;

TEST_CASE("cpu::transform::fft::apply2D()", "[assets][noa][cpu][transform]") {
    path_t path_base = test::PATH_NOA_DATA / "transform" / "fft";
    const YAML::Node& tests = YAML::LoadFile(path_base / "tests.yaml")["apply2D"]["tests"];
    io::ImageFile file;
    cpu::Stream stream;

    for (size_t i = 0; i < tests.size(); ++i) {
        INFO("test: " << i);

        const YAML::Node& test = tests[i];
        const auto path_input = path_base / test["input"].as<path_t>();
        const auto path_expected = path_base / test["expected"].as<path_t>();
        const auto scale = test["scale"].as<float2_t>();
        const auto rotate = math::toRad(test["rotate"].as<float>());
        const auto center = test["center"].as<float2_t>();
        const auto shift = test["shift"].as<float2_t>();
        const auto cutoff = test["cutoff"].as<float>();
        const auto interp = test["interp"].as<InterpMode>();

        float22_t matrix(transform::rotate(rotate) *
                         transform::scale(1 / scale));
        matrix = math::inverse(matrix);

        // Load input:
        file.open(path_input, io::READ);
        const size3_t shape = file.shape();
        const size3_t shape_fft = shapeFFT(shape);
        const size2_t pitch{shape.x, shape.y};
        const size2_t pitch_fft{shape_fft.x, shape_fft.y};

        cpu::memory::PtrHost<float> input(elements(shape));
        file.readAll(input.get(), false);
        file.close();

        // Go to Fourier space:
        cpu::memory::PtrHost<cfloat_t> input_fft(elementsFFT(shape));
        cpu::fft::r2c(input.get(), input_fft.get(), shape, 1, stream);
        const auto weight = 1.f / static_cast<float>(input.elements());
        cpu::math::ewise(input_fft.get(), shape_fft, math::sqrt(weight), input_fft.get(), shape_fft,
                         shape_fft, 1, noa::math::multiply_t{}, stream);

        // Apply new geometry:
        const size2_t shape_2d = {shape.x, shape.y};
        cpu::memory::PtrHost<cfloat_t> input_fft_centered(input_fft.elements());
        cpu::memory::PtrHost<cfloat_t> output_fft(input_fft.elements());
        cpu::transform::fft::shift2D<fft::H2HC>(
                input_fft.get(), pitch_fft, input_fft_centered.get(), pitch_fft, shape_2d, -center, 1, stream);
        cpu::transform::fft::apply2D<fft::HC2H>(
                input_fft_centered.get(), pitch_fft.x, output_fft.get(), pitch_fft.x, shape_2d,
                matrix, center + shift, cutoff, interp, stream);

        // Go back to real space:
        cpu::fft::c2r(output_fft.get(), input.get(), shape, 1, stream);
        cpu::math::ewise(input.get(), shape, math::sqrt(weight), input.get(), shape,
                         shape, 1, noa::math::multiply_t{}, stream);

        // Load excepted and compare
        cpu::memory::PtrHost<float> expected(input.elements());
        file.open(path_expected, io::READ);
        file.readAll(expected.get(), false);
        file.close();
        test::Matcher<float> matcher(test::MATCH_ABS, input.get(), expected.get(), input.elements(), 1e-6);
        REQUIRE(matcher);
    }
}

TEMPLATE_TEST_CASE("cpu::transform::fft::apply2D(), remap", "[noa][cpu][transform]", float, double) {
    const size_t batches = 3;
    const size3_t shape = {255, 256, 1};
    const size3_t pitch = shapeFFT(shape);
    const size2_t shape_2d = {shape.x, shape.y};
    const size2_t pitch_2d = {pitch.x, pitch.y};
    const float cutoff = 0.5;
    const auto interp = INTERP_LINEAR;
    cpu::Stream stream;

    test::Randomizer<float> randomizer(-3, 3);
    cpu::memory::PtrHost<float22_t> transforms(batches);
    cpu::memory::PtrHost<float2_t> shifts(batches);
    for (size_t batch = 0; batch < batches; ++batch) {
        const float2_t scale = {0.9, 1.1};
        const float rotate = randomizer.get();
        float22_t matrix(transform::rotate(rotate) *
                         transform::scale(1 / scale));
        transforms[batch] = math::inverse(matrix);
        shifts[batch] = {randomizer.get() * 10, randomizer.get() * 10};
    }

    using complex_t = Complex<TestType>;
    cpu::memory::PtrHost<complex_t> input(elementsFFT(shape) * batches);
    test::randomize(input.get(), input.elements(), randomizer);

    cpu::memory::PtrHost<complex_t> output_fft(input.elements());
    cpu::transform::fft::apply2D<fft::HC2H>(
            input.get(), pitch_2d, output_fft.get(), pitch_2d, shape_2d,
            transforms.get(), shifts.get(), batches, cutoff, interp, stream);

    cpu::memory::PtrHost<complex_t> output_fft_centered(input.elements());
    cpu::transform::fft::apply2D<fft::HC2HC>(
            input.get(), pitch_2d, output_fft_centered.get(), pitch_2d, shape_2d,
            transforms.get(), shifts.get(), batches, cutoff, interp, stream);
    cpu::fft::remap(fft::HC2H, output_fft_centered.get(), pitch, input.get(), pitch, shape, batches, stream);

    test::Matcher<complex_t> matcher(test::MATCH_ABS, input.get(), output_fft.get(), input.elements(), 1e-7);
    REQUIRE(matcher);
}

TEST_CASE("cpu::transform::fft::apply3D()", "[assets][noa][cpu][transform]") {
    path_t path_base = test::PATH_NOA_DATA / "transform" / "fft";
    const YAML::Node& tests = YAML::LoadFile(path_base / "tests.yaml")["apply3D"]["tests"];

    io::ImageFile file;
    cpu::Stream stream;
    for (size_t i = 0; i < tests.size(); ++i) {
        INFO("test: " << i);
        const YAML::Node& test = tests[i];
        const auto path_input = path_base / test["input"].as<path_t>();
        const auto path_expected = path_base / test["expected"].as<path_t>();
        const auto scale = test["scale"].as<float3_t>();
        const auto rotate = transform::toMatrix(math::toRad(test["rotate"].as<float3_t>()));
        const auto center = test["center"].as<float3_t>();
        const auto shift = test["shift"].as<float3_t>();
        const auto cutoff = test["cutoff"].as<float>();
        const auto interp = test["interp"].as<InterpMode>();

        float33_t matrix(rotate * transform::scale(1 / scale));
        matrix = math::inverse(matrix);

        // Load input:
        file.open(path_input, io::READ);
        const size3_t shape = file.shape();
        const size3_t shape_fft = shapeFFT(shape);
        const size2_t pitch{shape.x, shape.y};
        const size2_t pitch_fft{shape_fft.x, shape_fft.y};

        cpu::memory::PtrHost<float> input(elements(shape));
        file.readAll(input.get(), false);

        // Go to Fourier space:
        cpu::memory::PtrHost<cfloat_t> input_fft(elementsFFT(shape));
        cpu::fft::r2c(input.get(), input_fft.get(), shape, 1, stream);
        const auto weight = 1.f / static_cast<float>(input.elements());
        cpu::math::ewise(input_fft.get(), shape_fft, math::sqrt(weight), input_fft.get(), shape_fft,
                         shape_fft, 1, noa::math::multiply_t{}, stream);

        // Apply new geometry:
        cpu::memory::PtrHost<cfloat_t> input_fft_centered(input_fft.elements());
        cpu::memory::PtrHost<cfloat_t> output_fft(input_fft.elements());
        cpu::transform::fft::shift3D<fft::H2HC>(
                input_fft.get(), shape_fft, input_fft_centered.get(), shape_fft, shape, -center, 1, stream);
        cpu::transform::fft::apply3D<fft::HC2H>(
                input_fft_centered.get(), pitch_fft, output_fft.get(), pitch_fft, shape,
                matrix, center + shift, cutoff, interp, stream);

        // Go back to real space:
        cpu::fft::c2r(output_fft.get(), input.get(), shape, 1, stream);
        cpu::math::ewise(input.get(), shape, math::sqrt(weight), input.get(), shape,
                         shape, 1, noa::math::multiply_t{}, stream);

        // Load excepted and compare
        cpu::memory::PtrHost<float> expected(input.elements());
        file.open(path_expected, io::READ);
        file.readAll(expected.get());
        test::Matcher<float> matcher(test::MATCH_ABS, input.get(), expected.get(), input.elements(), 5e-6);
        REQUIRE(matcher);
    }
}

TEMPLATE_TEST_CASE("cpu::transform::fft::apply3D(), remap", "[noa][cpu][transform]", float, double) {
    const size_t batches = 3;
    const size3_t shape = {160, 161, 160};
    const size3_t pitch = shapeFFT(shape);
    const float cutoff = 0.5;
    const auto interp = INTERP_LINEAR;
    cpu::Stream stream;

    test::Randomizer<float> randomizer(-3, 3);
    cpu::memory::PtrHost<float33_t> transforms(batches);
    cpu::memory::PtrHost<float3_t> shifts(batches);
    for (size_t batch = 0; batch < batches; ++batch) {
        const float3_t scale = {0.9, 1.1, 1};
        float33_t matrix(transform::toMatrix(float3_t{randomizer.get(), randomizer.get(), randomizer.get()}) *
                         transform::scale(1 / scale));
        transforms[batch] = math::inverse(matrix);
        shifts[batch] = {randomizer.get() * 10, randomizer.get() * 10, randomizer.get() * 10};
    }

    using complex_t = Complex<TestType>;
    cpu::memory::PtrHost<complex_t> input(elementsFFT(shape) * batches);
    test::randomize(input.get(), input.elements(), randomizer);

    cpu::memory::PtrHost<complex_t> output_fft(input.elements());
    cpu::transform::fft::apply3D<fft::HC2H>(
            input.get(), pitch, output_fft.get(), pitch, shape,
            transforms.get(), shifts.get(), batches, cutoff, interp, stream);

    cpu::memory::PtrHost<complex_t> output_fft_centered(input.elements());
    cpu::transform::fft::apply3D<fft::HC2HC>(
            input.get(), pitch, output_fft_centered.get(), pitch, shape,
            transforms.get(), shifts.get(), batches, cutoff, interp, stream);
    cpu::fft::remap(fft::HC2H, output_fft_centered.get(), pitch, input.get(), pitch, shape, batches, stream);

    test::Matcher<complex_t> matcher(test::MATCH_ABS, input.get(), output_fft.get(), input.elements(), 1e-7);
    REQUIRE(matcher);
}
