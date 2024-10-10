#include <noa/core/geometry/Transform.hpp>
#include <noa/core/geometry/Euler.hpp>
#include <noa/unified/fft/Remap.hpp>
#include <noa/unified/fft/Transform.hpp>
#include <noa/unified/geometry/TransformSpectrum.hpp>
#include <noa/unified/io/ImageFile.hpp>
#include <noa/unified/Texture.hpp>
#include <noa/unified/Complex.hpp>
#include <noa/unified/Random.hpp>
#include <noa/unified/Factory.hpp>
#include <noa/unified/signal/PhaseShift.hpp>

#include <catch2/catch.hpp>
#include "Assets.h"
#include "Utils.hpp"
#include "noa/unified/geometry/DrawShape.hpp"

using namespace ::noa::types;
using Interp = noa::Interp;
using Remap = noa::Remap;

// TODO Surprisingly this is quite low precision. The images look good, but the errors can get quite
//      large. Of course, we are measure a round of r2c, phase-shift x2 and transformation... Instead,
//      we should measure the transformation step and only the transformation step. Also, there's still
//      this bug in the library at Nyquist, but here we don't even measure that because of the 0.45 cutoff.

TEST_CASE("unified::geometry::transform_spectrum_2d", ".") {
    constexpr auto shape = Shape4<i64>{1, 1, 256, 256};

    auto data = noa::empty<f32>(shape);
    noa::geometry::draw_shape({}, data, noa::geometry::Rectangle{
                                  .center = Vec{128., 128.},
                                  .radius = Vec{32., 44.},
                                  .smoothness = 80.,
                              });
    auto input_rfft = noa::fft::remap(Remap::FC2H, data, shape);

    i64 batch = 5;
    auto rotations = noa::empty<Mat22<f32>>(batch);
    for (f32 i{5}; auto& rotation: rotations.span_1d_contiguous()) {
        rotation = noa::geometry::rotate(noa::deg2rad(i));
        i += 10;
    }

    auto output_rfft = noa::empty<f32>(shape.set<0>(batch).rfft());
    noa::geometry::transform_spectrum_2d<Remap::H2HC>(
        input_rfft, output_rfft, shape,
        rotations, {}, {.interp = Interp::CUBIC_FAST, .fftfreq_cutoff = 1});

    noa::io::write(input_rfft, test::NOA_DATA_PATH / "geometry" / "test_input_rfft.mrc");
    noa::io::write(output_rfft, test::NOA_DATA_PATH / "geometry" / "test_output_rfft.mrc");
}

TEST_CASE("unified::geometry::fft::transform_2d, vs scipy", "[noa][unified][asset]") {
    const Path path_base = test::NOA_DATA_PATH / "geometry" / "fft";
    const YAML::Node param = YAML::LoadFile(path_base / "tests.yaml")["transform_2d"];

    std::vector<Device> devices{"cpu"};
    if (Device::is_any_gpu())
        devices.emplace_back("gpu");

    const size_t expected_count = param["tests"].size() * devices.size();
    REQUIRE(expected_count > 1);
    size_t count{};
    for (size_t nb{}; nb < param["tests"].size(); ++nb) {
        INFO("test number = " << nb);

        const YAML::Node& test = param["tests"][nb];
        const auto input_filename = path_base / test["input"].as<Path>();
        const auto expected_filename = path_base / test["expected"].as<Path>();
        const auto scale = test["scale"].as<Vec2<f64>>();
        const auto rotate = noa::deg2rad(test["rotate"].as<f64>());
        const auto center = test["center"].as<Vec2<f32>>();
        const auto shift = test["shift"].as<Vec2<f32>>();
        const auto cutoff = test["cutoff"].as<f32>();
        const auto interp = test["interp"].as<Interp>();
        constexpr auto FFT_NORM = noa::fft::Norm::ORTHO;

        const auto matrix = (noa::geometry::rotate(rotate) * noa::geometry::scale(1 / scale)).inverse().as<f32>();

        for (auto& device: devices) {
            auto stream = noa::StreamGuard(device);
            const auto options = noa::ArrayOption(device, noa::Allocator::MANAGED);
            INFO(device);

            const auto input = noa::io::read_data<f32>(input_filename, {.enforce_2d_stack = true}, options);
            const auto expected = noa::io::read_data<f32>(expected_filename, {.enforce_2d_stack = true}, options);
            const auto output = noa::like(expected);
            const auto input_fft = noa::empty<c32>(input.shape().rfft(), options);
            const auto input_fft_centered = noa::like(input_fft);
            const auto output_fft = noa::like(input_fft_centered);

            noa::fft::r2c(input, input_fft, {.norm=FFT_NORM});
            noa::signal::phase_shift_2d<Remap::H2HC>(input_fft, input_fft_centered, input.shape(), -center, cutoff);

            // With arrays:
            noa::geometry::transform_spectrum_2d<Remap::HC2H>(
                input_fft_centered, output_fft, input.shape(),
                matrix, center + shift, {interp, cutoff});

            noa::fft::c2r(output_fft, output, {.norm=FFT_NORM});
            REQUIRE(test::allclose_abs_safe(expected, output, 1e-4f));

            ++count;

            // With textures:
            noa::fill(output, 0); // erase
            const auto input_fft_centered_texture = Texture<c32>(input_fft_centered, device, interp);
            noa::geometry::transform_spectrum_2d<Remap::HC2H>(
                input_fft_centered_texture, output_fft, input.shape(),
                matrix, center + shift, {.fftfreq_cutoff = cutoff});
            noa::fft::c2r(output_fft, output, {.norm=FFT_NORM});
            REQUIRE(test::allclose_abs_safe(expected, output, 1e-4f));
        }
    }
    REQUIRE(count == expected_count);
}

// TEST_CASE("unified::geometry::fft::transform_3d, vs scipy", "[noa][unified][asset]") {
//     const Path path_base = test::NOA_DATA_PATH / "geometry" / "fft";
//     const YAML::Node param = YAML::LoadFile(path_base / "tests.yaml")["transform_3d"];
//
//     std::vector<Device> devices{"cpu"};
//     if (Device::is_any_gpu())
//         devices.emplace_back("gpu");
//
//     const size_t expected_count = param["tests"].size() * devices.size();
//     REQUIRE(expected_count > 1);
//     size_t count{};
//     for (size_t nb{}; nb < param["tests"].size(); ++nb) {
//         INFO("test number = " << nb);
//
//         const YAML::Node& test = param["tests"][nb];
//         const auto input_filename = path_base / test["input"].as<Path>();
//         const auto expected_filename = path_base / test["expected"].as<Path>();
//         const auto scale = test["scale"].as<Vec3<f64>>();
//         const auto rotate = noa::deg2rad(test["rotate"].as<Vec3<f64>>());
//         const auto center = test["center"].as<Vec3<f32>>();
//         const auto shift = test["shift"].as<Vec3<f32>>();
//         const auto cutoff = test["cutoff"].as<f32>();
//         const auto interp = test["interp"].as<Interp>();
//         constexpr auto FFT_NORM = noa::fft::Norm::ORTHO;
//
//         const auto matrix = (noa::geometry::euler2matrix(rotate) * noa::geometry::scale(1 / scale)).inverse().as<f32>();
//
//         for (auto& device: devices) {
//             const auto stream = StreamGuard(device);
//             const auto options = ArrayOption(device, Allocator::MANAGED);
//             INFO(device);
//
//             const auto input = noa::io::read_data<f32>(input_filename, {}, options);
//             const auto expected = noa::io::read_data<f32>(expected_filename, {}, options);
//             const auto output = noa::like(expected);
//             const auto input_fft = noa::empty<c32>(input.shape().rfft(), options);
//             const auto input_fft_centered = noa::like(input_fft);
//             const auto output_fft = noa::like(input_fft_centered);
//
//             noa::fft::r2c(input, input_fft, {.norm=FFT_NORM});
//             noa::signal::phase_shift_3d<Remap::H2HC>(input_fft, input_fft_centered, input.shape(), -center, cutoff);
//
//             // With arrays:
//             noa::geometry::transform_spectrum_3d<Remap::HC2H>(
//                 input_fft_centered, output_fft, input.shape(),
//                 matrix, center + shift, {interp, cutoff});
//             noa::fft::c2r(output_fft, output, {.norm=FFT_NORM});
//             REQUIRE(test::allclose_abs_safe(expected, output, 5e-3f)); // it was MATCH_ABS at 2e-4
//
//             ++count;
//
//             // With textures:
//             noa::fill(output, 0); // erase
//             const auto input_fft_centered_texture = Texture<c32>(input_fft_centered, device, interp);
//             noa::geometry::transform_spectrum_3d<Remap::HC2H>(
//                 input_fft_centered_texture, output_fft, input.shape(),
//                 matrix, center + shift, {.fftfreq_cutoff = cutoff});
//             noa::fft::c2r(output_fft, output, {.norm=FFT_NORM});
//             REQUIRE(test::allclose_abs_safe(expected, output, 5e-3f));
//         }
//     }
//     REQUIRE(count == expected_count);
// }
//
// TEMPLATE_TEST_CASE("unified::geometry::fft::transform_2d(), remap", "[noa][unified]", f32, f64) {
//     const auto shape = test::random_shape_batched(2);
//     constexpr f64 cutoff = 0.5;
//     constexpr auto interp = Interp::LINEAR;
//
//     test::Randomizer<f64> randomizer(-3, 3);
//     auto transforms = Array<Mat22<f32>>(shape[0]);
//     auto shifts = Array<Vec2<f32>>(shape[0]);
//
//     for (auto& transform: transforms.span_1d()) {
//         const auto scale = Vec{0.9, 1.1};
//         const f64 angle = randomizer.get();
//         transform = (noa::geometry::rotate(angle) * noa::geometry::scale(1 / scale)).inverse().as<f32>();
//     }
//     for (auto& shift: shifts.span_1d())
//         shift = Vec{randomizer.get() * 10, randomizer.get() * 10}.as<f32>();
//
//     std::vector<Device> devices{"cpu"};
//     if (Device::is_any_gpu())
//         devices.emplace_back("gpu");
//
//     for (auto& device: devices) {
//         const auto stream = StreamGuard(device);
//         const auto options = ArrayOption(device, Allocator::MANAGED);
//         INFO(device);
//
//         if (device != transforms.device()) {
//             transforms = transforms.to({device});
//             shifts = shifts.to({device});
//         }
//
//         const auto input = noa::random(noa::Uniform<TestType>{-3, 3}, shape, options);
//         const auto input_fft = noa::fft::r2c(input);
//
//         using complex_t = Complex<TestType>;
//         const auto output_fft = noa::empty<complex_t>(shape.rfft(), options);
//         noa::geometry::transform_spectrum_2d<Remap::HC2H>(
//             input_fft, output_fft, shape, transforms, shifts, {interp, cutoff});
//
//         if constexpr (std::is_same_v<TestType, f64>) {
//             const auto output_fft_centered = noa::like(output_fft);
//             noa::geometry::transform_spectrum_2d<Remap::HC2HC>(
//                 input_fft, output_fft_centered, shape,
//                 transforms, shifts, {interp, cutoff});
//             const auto output_fft_final = noa::fft::remap(Remap::HC2H, output_fft_centered, shape);
//             REQUIRE(test::allclose_abs(output_fft, output_fft_final, 1e-7));
//         } else {
//             const auto input_fft_texture = Texture<c32>(input_fft, device, interp);
//             const auto output_fft_centered = noa::like(output_fft);
//             noa::geometry::transform_spectrum_2d<Remap::HC2HC>(
//                 input_fft, output_fft_centered, shape,
//                 transforms, shifts, {interp, cutoff});
//             const auto output_fft_final = noa::fft::remap(Remap::HC2H, output_fft_centered, shape);
//             REQUIRE(test::allclose_abs(output_fft, output_fft_final, 1e-7));
//         }
//     }
// }
//
// TEMPLATE_TEST_CASE("unified::geometry::fft::transform_3d(), remap", "[noa][unified]", f32, f64) {
//     const auto shape = test::random_shape(3);
//     constexpr f64 cutoff = 0.5;
//     constexpr auto interp = Interp::LINEAR;
//
//     test::Randomizer<f64> randomizer(-3, 3);
//     auto transforms = Array<Mat33<f32>>(shape[0]);
//     auto shifts = Array<Vec3<f32>>(shape[0]);
//
//     for (auto& transform: transforms.span_1d()) {
//         const auto scale = Vec{0.9, 1.1, 0.85};
//         const auto angles = Vec{randomizer.get(), randomizer.get(), randomizer.get()};
//         transform = (noa::geometry::euler2matrix(angles) * noa::geometry::scale(1 / scale)).inverse().as<f32>();
//     }
//     for (auto& shift: shifts.span_1d()) {
//         shift = Vec{
//             randomizer.get() * 10,
//             randomizer.get() * 10,
//             randomizer.get() * 10,
//         }.as<f32>();
//     }
//
//     std::vector<Device> devices{"cpu"};
//     if (Device::is_any_gpu())
//         devices.emplace_back("gpu");
//
//     for (auto& device: devices) {
//         const auto stream = StreamGuard(device);
//         const auto options = ArrayOption(device, Allocator::MANAGED);
//         INFO(device);
//
//         if (device != transforms.device()) {
//             transforms = transforms.to({device});
//             shifts = shifts.to({device});
//         }
//
//         const auto input = noa::random(noa::Uniform<TestType>{-3, 3}, shape, options);
//         const auto input_fft = noa::fft::r2c(input);
//
//         using complex_t = Complex<TestType>;
//         const auto output_fft = noa::empty<complex_t>(shape.rfft(), options);
//         noa::geometry::transform_spectrum_3d<Remap::HC2H>(
//             input_fft, output_fft, shape, transforms, shifts, {interp, cutoff});
//
//         if constexpr (std::is_same_v<TestType, f64>) {
//             const auto output_fft_centered = noa::like(output_fft);
//             noa::geometry::transform_spectrum_3d<Remap::HC2HC>(
//                 input_fft, output_fft_centered, shape,
//                 transforms, shifts, {interp, cutoff});
//             const auto output_fft_final = noa::fft::remap(Remap::HC2H, output_fft_centered, shape);
//             REQUIRE(test::allclose_abs(output_fft, output_fft_final, 1e-7));
//         } else {
//             const auto input_fft_texture = noa::Texture<c32>(input_fft, device, interp);
//             const auto output_fft_centered = noa::like(output_fft);
//             noa::geometry::transform_spectrum_3d<Remap::HC2HC>(
//                 input_fft, output_fft_centered, shape,
//                 transforms, shifts, {interp, cutoff});
//             const auto output_fft_final = noa::fft::remap(Remap::HC2H, output_fft_centered, shape);
//             REQUIRE(test::allclose_abs(output_fft, output_fft_final, 1e-7));
//         }
//     }
// }
//
// // The hermitian symmetry isn't broken by transform_2d and transform_3d.
// TEST_CASE("unified::geometry::fft::transform_2d, check redundancy", "[.]") {
//     const auto shape = Shape4<i64>{1, 1, 128, 128};
//     const Path output_path = test::NOA_DATA_PATH / "geometry" / "fft";
//     const auto option = ArrayOption("gpu", Allocator::MANAGED);
//
//     Array input = noa::linspace(shape, noa::Linspace<f32>{-10, 10, true}, option);
//     Array output0 = noa::fft::r2c(input);
//     noa::fft::remap(Remap::H2HC, output0, output0, shape);
//
//     Array output1 = noa::like(output0);
//     const Mat22 rotation = noa::geometry::rotate(noa::deg2rad(45.f));
//     noa::geometry::transform_spectrum_2d<Remap::HC2HC>(output0, output1, shape, rotation);
//     noa::io::write(noa::real(output1), output_path / "test_output1_real.mrc");
//     noa::io::write(noa::imag(output1), output_path / "test_output1_imag.mrc");
// }
//
// TEST_CASE("unified::geometry::fft::transform_3d, check redundancy", "[.]") {
//     const auto shape = Shape4<i64>{1, 128, 128, 128};
//     const Path output_path = test::NOA_DATA_PATH / "geometry" / "fft";
//     const auto option = ArrayOption(Device("gpu"), Allocator::MANAGED);
//
//     Array input = noa::linspace(shape, noa::Linspace<f32>{-10, 10, true}, option);
//     Array output0 = noa::fft::r2c(input);
//     noa::fft::remap(noa::Remap::H2HC, output0, output0, shape);
//
//     Array output1 = noa::like(output0);
//     const Mat33 rotation = noa::geometry::euler2matrix(noa::deg2rad(Vec{45.f, 0.f, 0.f}), {.axes="zyx", .intrinsic = false});
//     noa::geometry::transform_spectrum_3d<Remap::HC2HC>(output0, output1, shape, rotation, Vec3<f32>{});
//     noa::io::write(noa::real(output1), output_path / "test_output1_real.mrc");
//     noa::io::write(noa::imag(output1), output_path / "test_output1_imag.mrc");
// }
