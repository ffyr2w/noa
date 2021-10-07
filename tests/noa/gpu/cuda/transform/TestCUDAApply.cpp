#include <noa/common/files/MRCFile.h>
#include <noa/common/transform/Geometry.h>
#include <noa/common/transform/Euler.h>

#include <noa/cpu/memory/PtrHost.h>
#include <noa/cpu/math/Arithmetics.h>
#include <noa/cpu/math/Reductions.h>
#include <noa/gpu/cuda/memory/PtrDevicePadded.h>
#include <noa/gpu/cuda/memory/Copy.h>
#include <noa/gpu/cuda/transform/Apply.h>

#include "Assets.h"
#include "Helpers.h"
#include <catch2/catch.hpp>

// TestCUDARotate is checking against the CPU backend for all InterpMode and BorderMode.
// Since rotate() is calling apply(), here tests are only checking against assets.

using namespace ::noa;

TEST_CASE("cuda::transform::apply2D()", "[assets][noa][cuda][transform]") {
    path_t path_base = test::PATH_TEST_DATA / "transform";
    YAML::Node param = YAML::LoadFile(path_base / "param.yaml")["apply2D"];
    auto input_filename = path_base / param["input"].as<path_t>();

    auto center = param["center"].as<float2_t>();
    auto scale = param["scale"].as<float2_t>();
    auto rotate = math::toRad(param["rotate"].as<float>());
    auto shift = param["shift"].as<float2_t>();
    float33_t matrix(transform::translate(center) *
                     transform::translate(shift) *
                     float33_t(transform::rotate(rotate)) *
                     float33_t(transform::scale(scale)) *
                     transform::translate(-center));
    matrix = math::inverse(matrix);

    MRCFile file;
    for (size_t nb = 0; nb < param["tests"].size(); ++nb) {
        INFO("test number = " << nb);

        const YAML::Node& test = param["tests"][nb];
        auto expected_filename = path_base / test["expected"].as<path_t>();
        auto interp = test["interp"].as<InterpMode>();
        auto border = test["border"].as<BorderMode>();

        // Some BorderMode, or BorderMode-InterpMode combination, are not supported on the CUDA implementations.
        if (border == BORDER_VALUE || border == BORDER_REFLECT)
            continue;
        else if (border == BORDER_MIRROR || border == BORDER_PERIODIC)
            if (interp != INTERP_LINEAR_FAST && interp != INTERP_NEAREST)
                continue;

        // Get input.
        file.open(input_filename, io::READ);
        size3_t shape = file.getShape();
        size_t elements = getElements(shape);
        cpu::memory::PtrHost<float> input(elements);
        file.readAll(input.get());

        // Get expected.
        cpu::memory::PtrHost<float> expected(elements);
        file.open(expected_filename, io::READ);
        file.readAll(expected.get());

        cuda::Stream stream;
        cpu::memory::PtrHost<float> output(elements);
        cuda::memory::PtrDevicePadded<float> d_input(shape);
        { // 3x3
            cuda::memory::copy(input.get(), shape.x, d_input.get(), d_input.pitch(), shape, stream);
            cuda::transform::apply2D(d_input.get(), d_input.pitch(), {shape.x, shape.y},
                                     d_input.get(), d_input.pitch(), {shape.x, shape.y},
                                     matrix, interp, border, stream);
            cuda::memory::copy(d_input.get(), d_input.pitch(), output.get(), shape.x, shape, stream);
            stream.synchronize();

            if (interp == INTERP_LINEAR) {
                cpu::math::subtractArray(expected.get(), output.get(), output.get(), elements, 1);
                float min, max, mean;
                cpu::math::minMaxSumMean<float>(output.get(), &min, &max, nullptr, &mean, elements, 1);
                REQUIRE(math::abs(min) < 1e-2f); // don't know what to think about these values
                REQUIRE(math::abs(max) < 1e-2f);
                REQUIRE(math::abs(mean) < 1e-5f);
            } else {
                float diff = test::getDifference(expected.get(), output.get(), elements);
                REQUIRE_THAT(diff, test::isWithinAbs(0.f, 1e-6));
            }
        }

        { // 2x3
            cuda::memory::copy(input.get(), shape.x, d_input.get(), d_input.pitch(), shape, stream);
            cuda::transform::apply2D(d_input.get(), d_input.pitch(), {shape.x, shape.y},
                                     d_input.get(), d_input.pitch(), {shape.x, shape.y},
                                     float23_t(matrix), interp, border, stream);
            cuda::memory::copy(d_input.get(), d_input.pitch(), output.get(), shape.x, shape, stream);
            stream.synchronize();

            if (interp == INTERP_LINEAR) {
                cpu::math::subtractArray(expected.get(), output.get(), output.get(), elements, 1);
                float min, max, mean;
                cpu::math::minMaxSumMean<float>(output.get(), &min, &max, nullptr, &mean, elements, 1);
                REQUIRE(math::abs(min) < 5e-4f);
                REQUIRE(math::abs(max) < 5e-4f);
                REQUIRE(math::abs(mean) < 1e-6f);
            } else {
                float diff = test::getDifference(expected.get(), output.get(), elements);
                REQUIRE_THAT(diff, test::isWithinAbs(0.f, 1e-6));
            }
        }
    }
}

TEST_CASE("cuda::transform::apply3D()", "[assets][noa][cuda][transform]") {
    path_t path_base = test::PATH_TEST_DATA / "transform";
    YAML::Node param = YAML::LoadFile(path_base / "param.yaml")["apply3D"];
    auto input_filename = path_base / param["input"].as<path_t>();

    auto center = param["center"].as<float3_t>();
    auto scale = param["scale"].as<float3_t>();
    auto euler = math::toRad(param["euler"].as<float3_t>());
    auto shift = param["shift"].as<float3_t>();
    float44_t matrix(transform::translate(center) *
                     transform::translate(shift) *
                     float44_t(transform::toMatrix(euler)) *
                     float44_t(transform::scale(scale)) *
                     transform::translate(-center));
    matrix = math::inverse(matrix);

    MRCFile file;
    for (size_t nb = 0; nb < param["tests"].size(); ++nb) {
        INFO("test number = " << nb);

        const YAML::Node& test = param["tests"][nb];
        auto expected_filename = path_base / test["expected"].as<path_t>();
        auto interp = test["interp"].as<InterpMode>();
        auto border = test["border"].as<BorderMode>();

        // Some BorderMode, or BorderMode-InterpMode combination, are not supported on the CUDA implementations.
        if (border == BORDER_VALUE || border == BORDER_REFLECT)
            continue;
        else if (border == BORDER_MIRROR || border == BORDER_PERIODIC)
            if (interp != INTERP_LINEAR_FAST && interp != INTERP_NEAREST)
                continue;

        // Get input.
        file.open(input_filename, io::READ);
        size3_t shape = file.getShape();
        size_t elements = getElements(shape);
        cpu::memory::PtrHost<float> input(elements);
        file.readAll(input.get());

        // Get expected.
        cpu::memory::PtrHost<float> expected(elements);
        file.open(expected_filename, io::READ);
        file.readAll(expected.get());

        cuda::Stream stream;
        cpu::memory::PtrHost<float> output(elements);
        cuda::memory::PtrDevicePadded<float> d_input(shape);
        { // 4x4
            cuda::memory::copy(input.get(), shape.x, d_input.get(), d_input.pitch(), shape, stream);
            cuda::transform::apply3D(d_input.get(), d_input.pitch(), shape,
                                     d_input.get(), d_input.pitch(), shape,
                                     matrix, interp, border, stream);
            cuda::memory::copy(d_input.get(), d_input.pitch(), output.get(), shape.x, shape, stream);
            stream.synchronize();

            if (interp == INTERP_LINEAR) {
                cpu::math::subtractArray(expected.get(), output.get(), output.get(), elements, 1);
                float min, max, mean;
                cpu::math::minMaxSumMean<float>(output.get(), &min, &max, nullptr, &mean, elements, 1);
                REQUIRE(math::abs(min) < 5e-4f);
                REQUIRE(math::abs(max) < 5e-4f);
                REQUIRE(math::abs(mean) < 1e-6f);
            } else {
                float diff = test::getDifference(expected.get(), output.get(), elements);
                REQUIRE_THAT(diff, test::isWithinAbs(0.f, 1e-6));
            }
        }

        { // 3x4
            cuda::memory::copy(input.get(), shape.x, d_input.get(), d_input.pitch(), shape, stream);
            cuda::transform::apply3D(d_input.get(), d_input.pitch(), shape,
                                     d_input.get(), d_input.pitch(), shape,
                                     float34_t(matrix), interp, border, stream);
            cuda::memory::copy(d_input.get(), d_input.pitch(), output.get(), shape.x, shape, stream);
            stream.synchronize();

            if (interp == INTERP_LINEAR) {
                cpu::math::subtractArray(expected.get(), output.get(), output.get(), elements, 1);
                float min, max, mean;
                cpu::math::minMaxSumMean<float>(output.get(), &min, &max, nullptr, &mean, elements, 1);
                REQUIRE(math::abs(min) < 5e-4f);
                REQUIRE(math::abs(max) < 5e-4f);
                REQUIRE(math::abs(mean) < 1e-6f);
            } else {
                float diff = test::getDifference(expected.get(), output.get(), elements);
                REQUIRE_THAT(diff, test::isWithinAbs(0.f, 1e-6));
            }
        }
    }
}
