#include <noa/core/geometry/Euler.hpp>
#include <noa/unified/geometry/CubicBSplinePrefilter.hpp>
#include <noa/unified/geometry/DrawShape.hpp>
#include <noa/unified/geometry/Symmetry.hpp>
#include <noa/unified/geometry/Transform.hpp>
#include <noa/unified/io/ImageFile.hpp>
#include <noa/unified/Factory.hpp>
#include <noa/unified/Random.hpp>

#include <catch2/catch.hpp>
#include "Assets.h"
#include "Utils.hpp"

namespace ng = ::noa::geometry;
using namespace ::noa::types;
using Interp = noa::Interp;

TEST_CASE("unified::geometry::symmetrize_2d", "[noa][unified][assets]") {
    const Path path_base = test::NOA_DATA_PATH / "geometry";
    const YAML::Node param = YAML::LoadFile(path_base / "tests.yaml")["transform_2d_symmetry"];
    const auto input_filename = path_base / param["input"].as<Path>();

    constexpr bool COMPUTE_ASSETS = false;
    if constexpr (COMPUTE_ASSETS) {
        const auto asset = noa::empty<f32>({1, 1, 512, 512});
        const auto center = Vec{256., 256.};
        ng::draw_shape({}, asset, ng::Rectangle{
            .center = center,
            .radius = Vec{64., 128.},
            .smoothness = 5.,
        });
        ng::draw_shape(asset, asset, ng::Rectangle{
            .center = center + Vec{64., 128.},
            .radius = Vec{32., 32.},
            .smoothness = 3.,
        }, {}, noa::Plus{});
        noa::io::write(asset, input_filename);
    }

    std::vector<Device> devices{"cpu"};
    if (not COMPUTE_ASSETS and Device::is_any_gpu())
        devices.emplace_back("gpu");

    const size_t expected_count = param["tests"].size() * devices.size();
    REQUIRE(expected_count > 1);
    size_t count{};
    for (size_t nb{}; nb < param["tests"].size(); ++nb) {
        INFO("test number = " << nb);

        const YAML::Node& test = param["tests"][nb];
        const auto expected_filename = path_base / test["expected"].as<Path>();
        const auto angle = noa::deg2rad(test["angle"].as<f32>());
        const auto center = test["center"].as<Vec2<f64>>();
        const auto interp = test["interp"].as<Interp>();

        const auto inverse_pre_matrix = ng::translate(test["shift"].as<Vec2<f32>>());
        const auto inverse_post_matrix = ng::linear2affine(ng::rotate(-angle));
        auto symmetry = ng::Symmetry<f32, 2>(test["symmetry"].as<std::string>());

        for (auto& device: devices) {
            const auto stream = StreamGuard(device);
            const auto options = ArrayOption(device, Allocator::MANAGED);
            INFO(device);

            if (symmetry.device() != device)
                symmetry = symmetry.to({device});

            const auto input = noa::io::read_data<f32>(input_filename, {.enforce_2d_stack = true}, options);
            if (interp.is_almost_any(Interp::CUBIC_BSPLINE))
                ng::cubic_bspline_prefilter(input, input);

            // With arrays:
            const auto output = noa::like(input);
            ng::symmetrize_2d(
                input, output, symmetry,
                {.symmetry_center = center, .interp = interp},
                inverse_pre_matrix, inverse_post_matrix);

            if constexpr (COMPUTE_ASSETS) {
                noa::io::write(output, expected_filename);
                continue;
            }

            const auto expected = noa::io::read_data<f32>(expected_filename, {.enforce_2d_stack = true}, options);
            if (interp == noa::Interp::NEAREST) {
                // For nearest neighbour, the border can be off by one pixel,
                // so here just check the total difference.
                const test::MatchResult results = test::allclose_abs(expected, output, 1e-4f);
                REQUIRE_THAT(results.total_abs_diff, Catch::WithinAbs(0, 1e-6));
            } else {
                // Otherwise it is usually around 2e-5, but there are some outliers...
                REQUIRE(test::allclose_abs_safe(expected, output, 5e-4));
            }
            ++count;

            // With textures:
            const auto input_texture = noa::Texture<f32>(input, device, interp, {
                .border = noa::Border::ZERO,
                .cvalue = 0.f,
                .prefilter = false,
            });

            noa::fill(output, 0); // erase
            ng::symmetrize_2d(
                input_texture, output, symmetry,
                {.symmetry_center=center},
                inverse_pre_matrix, inverse_post_matrix);

            if (interp == noa::Interp::NEAREST) {
                const test::MatchResult results = test::allclose_abs(expected, output, 1e-4f);
                REQUIRE_THAT(results.total_abs_diff, Catch::WithinAbs(0, 1e-6));
            } else {
                REQUIRE(test::allclose_abs_safe(expected, output, 5e-4));
            }
        }
    }
    REQUIRE(count == expected_count);
}

TEST_CASE("unified::geometry::transform_3d, symmetry", "[noa][unified][assets]") {
    const Path path_base = test::NOA_DATA_PATH / "geometry";
    const YAML::Node param = YAML::LoadFile(path_base / "tests.yaml")["transform_3d_symmetry"];

    constexpr bool COMPUTE_ASSETS = false;
    if constexpr (COMPUTE_ASSETS) {
        const auto asset = noa::empty<f32>({1, 150, 150, 150});
        constexpr auto center = Vec<f64, 3>::from_value(150 / 2);
        ng::draw_shape({}, asset, ng::Rectangle{.center=center, .radius=Vec{34., 24., 24.}, .smoothness=3.});
        noa::io::write(asset, path_base / param["input"][0].as<Path>());

        ng::draw_shape(asset, asset, ng::Rectangle{
            .center = center + Vec{50., 34., 34.},
            .radius = Vec{15., 15., 15.},
            .smoothness = 3.
        }, {}, noa::Plus{});
        noa::io::write(asset, path_base / param["input"][1].as<Path>());
    }

    std::vector<Device> devices{"cpu"};
    if (not COMPUTE_ASSETS and Device::is_any_gpu())
        devices.emplace_back("gpu");

    const size_t expected_count = param["tests"].size() * devices.size();
    REQUIRE(expected_count > 1);
    size_t count{};
    for (size_t nb{}; nb < param["tests"].size(); ++nb) {
        INFO("test number = " << nb);

        const YAML::Node& test = param["tests"][nb];
        const auto input_filename = path_base / test["input"].as<Path>();
        const auto expected_filename = path_base / test["expected"].as<Path>();
        const auto shift = test["shift"].as<Vec3<f32>>();
        const auto angles = noa::deg2rad(test["angles"].as<Vec3<f32>>());
        const auto center = test["center"].as<Vec3<f64>>();
        const auto interp = test["interp"].as<Interp>();

        auto symmetry = ng::Symmetry<f32, 3>(test["symmetry"].as<std::string>());
        const auto inverse_pre_matrix = noa::geometry::translate(shift);
        const auto inverse_post_matrix = ng::linear2affine(ng::euler2matrix(angles, {.axes="zyz"}).transpose());

        for (auto& device: devices) {
            const auto stream = noa::StreamGuard(device);
            const auto options = noa::ArrayOption(device, noa::Allocator::MANAGED);
            INFO(device);

            const auto input = noa::io::read_data<f32>(input_filename, {.enforce_2d_stack = false}, options);
            if (interp.is_almost_any(Interp::CUBIC_BSPLINE))
                ng::cubic_bspline_prefilter(input, input);

            // With arrays:
            const auto output = noa::like(input);

            ng::symmetrize_3d(
                input, output, symmetry,
                {.symmetry_center=center, .interp=interp},
                inverse_pre_matrix, inverse_post_matrix);

            if constexpr (COMPUTE_ASSETS) {
                noa::io::write(output, expected_filename);
                continue;
            }

            const auto expected = noa::io::read_data<f32>(expected_filename, {.enforce_2d_stack = false}, options);
            if (interp == noa::Interp::NEAREST) {
                const test::MatchResult results = test::allclose_abs(expected, output, 1e-4f);
                REQUIRE_THAT(results.total_abs_diff, Catch::WithinAbs(0, 1e-6));
            } else {
                // Usually around 2e-5, but there are some outliers...
                REQUIRE(test::allclose_abs_safe(expected, output, 5e-4));
            }
            ++count;

            // With textures:
            const auto input_texture = noa::Texture<f32>(input, device, interp, {
                .border=noa::Border::ZERO,
                .cvalue=0.f,
                .prefilter=false,
            });
            noa::fill(output, 0.); // erase
            ng::symmetrize_3d(
                input_texture, output, symmetry,
                {.symmetry_center=center},
                inverse_pre_matrix, inverse_post_matrix);

            if (interp == noa::Interp::NEAREST) {
                const test::MatchResult results = test::allclose_abs(expected, output, 1e-4f);
                REQUIRE_THAT(results.total_abs_diff, Catch::WithinAbs(0, 1e-6));
            } else {
                REQUIRE(test::allclose_abs_safe(expected, output, 5e-4));
            }
        }
    }
    REQUIRE(count == expected_count);
}
