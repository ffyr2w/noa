#include <noa/common/io/MRCFile.h>
#include <noa/cpu/memory/PtrHost.h>
#include <noa/cpu/geometry/Transform.h>

#include "Assets.h"
#include "Helpers.h"
#include <catch2/catch.hpp>

using namespace ::noa;

TEST_CASE("cpu::geometry::rotate2D() -- vs scipy", "[assets][noa][cpu][geometry]") {
    const path_t path_base = test::NOA_DATA_PATH / "geometry";
    const YAML::Node param = YAML::LoadFile(path_base / "tests.yaml")["rotate2D"];
    const auto input_filename = path_base / param["input"].as<path_t>();
    const auto border_value = param["border_value"].as<float>();
    const auto rotate = math::deg2rad(param["rotate"].as<float>());
    const auto center = param["center"].as<float2_t>();
    const auto matrix =
            geometry::translate(center) *
            float33_t(geometry::rotate(-rotate)) *
            geometry::translate(-center);

    io::MRCFile file;
    cpu::Stream stream;
    for (size_t nb = 0; nb < param["tests"].size(); ++nb) {
        INFO("test number = " << nb);

        const YAML::Node& test = param["tests"][nb];
        const auto expected_filename = path_base / test["expected"].as<path_t>();
        const auto interp = test["interp"].as<InterpMode>();
        const auto border = test["border"].as<BorderMode>();

        // Get input.
        file.open(input_filename, io::READ);
        const size4_t shape = file.shape();
        const size4_t stride = shape.strides();
        const size_t elements = shape.elements();
        cpu::memory::PtrHost<float> input(elements);
        file.readAll(input.get());

        // Get expected.
        cpu::memory::PtrHost<float> expected(elements);
        file.open(expected_filename, io::READ);
        file.readAll(expected.get());

        cpu::memory::PtrHost<float> output(elements);
        cpu::geometry::transform2D(input.share(), stride, shape, output.share(), stride, shape,
                                   matrix, interp, border, border_value, true, stream);
        stream.synchronize();

        if (interp == INTERP_LINEAR) {
            // it is usually around 2e-5
            REQUIRE(test::Matcher(test::MATCH_ABS_SAFE, expected.get(), output.get(), elements, 1e-4f));
        } else {
            const float diff = test::getDifference(expected.get(), output.get(), elements);
            REQUIRE_THAT(diff, Catch::WithinAbs(0, 1e-6));
        }
    }
}

TEST_CASE("cpu::geometry::rotate3D()", "[assets][noa][cpu][geometry]") {
    const path_t path_base = test::NOA_DATA_PATH / "geometry";
    const YAML::Node param = YAML::LoadFile(path_base / "tests.yaml")["rotate3D"];
    const auto input_filename = path_base / param["input"].as<path_t>();
    const auto border_value = param["border_value"].as<float>();
    const auto euler = math::deg2rad(param["euler"].as<float3_t>());
    const auto center = param["center"].as<float3_t>();
    const auto matrix =
            geometry::translate(center) *
            float44_t(geometry::euler2matrix(euler).transpose()) *
            geometry::translate(-center);

    io::MRCFile file;
    cpu::Stream stream;
    for (size_t nb = 0; nb < param["tests"].size(); ++nb) {
        INFO("test number = " << nb);

        const YAML::Node& test = param["tests"][nb];
        const auto expected_filename = path_base / test["expected"].as<path_t>();
        const auto interp = test["interp"].as<InterpMode>();
        const auto border = test["border"].as<BorderMode>();

        // Get input.
        file.open(input_filename, io::READ);
        const size4_t shape = file.shape();
        const size4_t stride = shape.strides();
        const size_t elements = shape.elements();
        cpu::memory::PtrHost<float> input(elements);
        file.readAll(input.get());

        // Get expected.
        cpu::memory::PtrHost<float> expected(elements);
        file.open(expected_filename, io::READ);
        file.readAll(expected.get());

        cpu::memory::PtrHost<float> output(elements);
        cpu::geometry::transform3D(input.share(), stride, shape, output.share(), stride, shape,
                                   matrix, interp, border, border_value, true, stream);
        stream.synchronize();

        if (interp == INTERP_LINEAR) {
            REQUIRE(test::Matcher(test::MATCH_ABS_SAFE, expected.get(), output.get(), elements, 1e-4f));
        } else {
            const float diff = test::getDifference(expected.get(), output.get(), elements);
            REQUIRE_THAT(diff, Catch::WithinAbs(0, 1e-6));
        }
    }
}
