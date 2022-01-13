#include <noa/common/io/ImageFile.h>

#include <noa/common/transform/Euler.h>
#include <noa/common/transform/Geometry.h>

#include <noa/cpu/math/Ewise.h>
#include <noa/cpu/memory/PtrHost.h>
#include <noa/cpu/filter/Shape.h>
#include <noa/cpu/transform/Apply.h>

#include "Assets.h"
#include "Helpers.h"
#include <catch2/catch.hpp>

using namespace ::noa;

TEST_CASE("cpu::transform::apply2D() - symmetry", "[assets][noa][cpu][transform]") {
    path_t base_path = test::PATH_NOA_DATA / "transform";
    YAML::Node param = YAML::LoadFile(base_path / "tests.yaml")["apply2D_symmetry"];
    path_t input_path = base_path / param["input"].as<path_t>();
    io::ImageFile file;

    cpu::Stream stream;

    constexpr bool COMPUTE_ASSETS = false;
    if constexpr (COMPUTE_ASSETS) {
        size3_t shape{512, 512, 1};
        size_t elements = noa::elements(shape);
        cpu::memory::PtrHost<float> input(elements);

        cpu::filter::rectangle<false, float>(nullptr, shape, input.get(), shape, shape, 1,
                                             float3_t{shape / 2}, {128, 64, 1}, 5, stream);
        cpu::memory::PtrHost<float> tmp(elements);
        cpu::filter::rectangle<false, float>(nullptr, shape, tmp.get(), shape, shape, 1,
                                             float3_t{shape / 2 + size3_t{128, 64, 0}}, {32, 32, 1}, 3, stream);
        cpu::math::ewise(input.get(), shape, tmp.get(), shape, input.get(), shape,
                         shape, 1, noa::math::plus_t{}, stream);

        file.open(input_path, io::WRITE);
        file.shape(shape);
        file.writeAll(input.get(), false);
    }

    for (size_t i = 0; i < param["tests"].size(); ++i) {
        INFO("test number = " << i);

        // Parameters:
        auto current = param["tests"][i];
        auto filename_expected = base_path / current["expected"].as<path_t>();
        auto shift = current["shift"].as<float2_t>();
        float22_t matrix = transform::rotate(-math::toRad(current["angle"].as<float>())); // inverse
        transform::Symmetry symmetry(current["symmetry"].as<std::string>());
        auto center = current["center"].as<float2_t>();
        auto interp = current["interp"].as<InterpMode>();

        // Prepare data:
        file.open(input_path, io::READ);
        size3_t shape = file.shape();
        size2_t shape_2d = {shape.x, shape.y};
        size_t elements = noa::elements(shape);
        cpu::memory::PtrHost<float> input(elements);
        cpu::memory::PtrHost<float> output(elements);

        file.readAll(input.get());
        cpu::transform::apply2D(input.get(), shape.x, output.get(), shape.x, shape_2d,
                                shift, matrix, symmetry, center, interp, true, stream);

        if constexpr (COMPUTE_ASSETS) {
            file.open(filename_expected, io::WRITE);
            file.shape(shape);
            file.writeAll(output.get(), false);
        } else {
            cpu::memory::PtrHost<float> expected(elements);
            file.open(filename_expected, io::READ);
            file.readAll(expected.get());
            if (interp != INTERP_NEAREST) {
                REQUIRE(test::Matcher(test::MATCH_ABS_SAFE, expected.get(), output.get(), elements, 5e-4));
            } else {
                float diff = test::getDifference(expected.get(), output.get(), elements);
                REQUIRE_THAT(diff, Catch::WithinAbs(0, 1e-6));
            }
        }
    }
}

TEST_CASE("cpu::transform::apply3D() - symmetry", "[assets][noa][cpu][transform]") {
    path_t base_path = test::PATH_NOA_DATA / "transform";
    YAML::Node param = YAML::LoadFile(base_path / "tests.yaml")["apply3D_symmetry"];
    io::ImageFile file;

    cpu::Stream stream;

    constexpr bool COMPUTE_ASSETS = false;
    if constexpr (COMPUTE_ASSETS) {
        size3_t shape{150, 150, 150};
        size_t elements = noa::elements(shape);
        cpu::memory::PtrHost<float> input(elements);

        float3_t rectangle_center(shape / size_t{2});
        cpu::filter::rectangle<false, float>(nullptr, shape, input.get(), shape, shape, 1,
                                             rectangle_center, {24, 24, 34}, 3, stream);
        file.open(base_path / param["input"][0].as<path_t>(), io::WRITE);
        file.shape(shape);
        file.writeAll(input.get(), false);

        cpu::memory::PtrHost<float> tmp(elements);
        cpu::filter::rectangle<false, float>(nullptr, shape, tmp.get(), shape, shape, 1,
                                             rectangle_center + float3_t{34, 34, 50}, {15, 15, 15}, 3, stream);
        cpu::math::ewise(input.get(), shape, tmp.get(), shape, input.get(), shape,
                         shape, 1, noa::math::plus_t{}, stream);
        file.open(base_path / param["input"][1].as<path_t>(), io::WRITE);
        file.shape(shape);
        file.writeAll(input.get(), false);
    }

    for (size_t i = 0; i < param["tests"].size(); ++i) {
        INFO("test number = " << i);

        // Parameters:
        auto current = param["tests"][i];
        auto filename_expected = base_path / current["expected"].as<path_t>();
        auto filename_input = base_path / current["input"].as<path_t>();
        auto shift = current["shift"].as<float3_t>();
        float33_t matrix = transform::toMatrix<true>(math::toRad(current["angle"].as<float3_t>()));
        transform::Symmetry symmetry(current["symmetry"].as<std::string>());
        auto center = current["center"].as<float3_t>();
        auto interp = current["interp"].as<InterpMode>();

        // Prepare data:
        file.open(filename_input, io::READ);
        size3_t shape = file.shape();
        size2_t shape_2d = {shape.x, shape.y};
        size_t elements = noa::elements(shape);
        cpu::memory::PtrHost<float> input(elements);
        cpu::memory::PtrHost<float> output(elements);

        file.readAll(input.get());
        cpu::transform::apply3D(input.get(), shape_2d, output.get(), shape_2d, shape,
                                shift, matrix, symmetry, center, interp, true, stream);

        if constexpr (COMPUTE_ASSETS) {
            file.open(filename_expected, io::WRITE);
            file.shape(shape);
            file.writeAll(output.get(), false);
        } else {
            cpu::memory::PtrHost<float> expected(elements);
            file.open(filename_expected, io::READ);
            file.readAll(expected.get());
            if (interp != INTERP_NEAREST) {
                REQUIRE(test::Matcher(test::MATCH_ABS_SAFE, expected.get(), output.get(), elements, 5e-4));
            } else {
                float diff = test::getDifference(expected.get(), output.get(), elements);
                REQUIRE_THAT(diff, Catch::WithinAbs(0, 1e-6));
            }
        }
    }
}
