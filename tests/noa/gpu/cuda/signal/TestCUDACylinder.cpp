#include <noa/common/io/MRCFile.h>
#include <noa/common/geometry/Transform.h>
#include <noa/common/geometry/Euler.h>
#include <noa/cpu/signal/Shape.h>

#include <noa/gpu/cuda/memory/PtrManaged.h>
#include <noa/gpu/cuda/signal/Shape.h>

#include "Assets.h"
#include "Helpers.h"
#include <catch2/catch.hpp>

using namespace noa;

TEST_CASE("cuda::signal::cylinder(), 3D", "[assets][noa][cuda]") {
    test::Randomizer<float> randomizer(-5, 5);

    const path_t path_base = test::NOA_DATA_PATH / "signal";
    YAML::Node tests = YAML::LoadFile(path_base / "tests.yaml")["cylinder"]["tests3D"];
    io::MRCFile file;
    cuda::Stream stream;

    for (size_t nb = 0; nb < tests.size(); ++nb) {
        INFO("test number = " << nb);

        const YAML::Node& test = tests[nb];
        auto invert = test["invert"].as<bool>();
        const auto shape = test["shape"].as<size4_t>();
        const size4_t strides = shape.strides();
        const size_t elements = shape.elements();

        const auto center = test["center"].as<float3_t>();
        const auto radius = test["radius"].as<float>();
        const auto length = test["length"].as<float>();
        const auto taper = test["taper"].as<float>();
        const auto fwd_transform = geometry::euler2matrix(math::deg2rad(test["angle"].as<float3_t>()));

        const auto filename_expected = path_base / test["expected"].as<path_t>();
        file.open(filename_expected, io::READ);
        if (all(file.shape() != shape))
            FAIL("asset shape is not correct");
        cuda::memory::PtrManaged<float> mask_expected(elements, stream);
        file.readAll(mask_expected.get());

        cuda::memory::PtrManaged<float> input_expected(elements, stream);
        cuda::memory::PtrManaged<float> input_result(elements, stream);
        cuda::memory::PtrManaged<float> mask_result(elements, stream);

        // Test saving the mask.
        cuda::signal::cylinder<float>(nullptr, {}, mask_result.share(), strides, shape,
                                      center, radius, length, taper, math::inverse(fwd_transform), invert, stream);
        stream.synchronize();
        REQUIRE(test::Matcher(test::MATCH_ABS_SAFE, mask_expected.get(), mask_result.get(), elements, 1e-6));

        AND_THEN("invert = true") {
            test::randomize(input_expected.get(), elements, randomizer);
            test::copy(input_expected.get(), input_result.get(), elements);

            cuda::signal::cylinder(input_result.share(), strides, input_result.share(), strides, shape,
                                   center, radius, length, taper, math::inverse(fwd_transform), true, stream);
            for (size_t idx = 0; idx < elements; ++idx)
                input_expected[idx] *= invert ? mask_expected[idx] : 1 - mask_expected[idx];

            stream.synchronize();
            REQUIRE(test::Matcher(test::MATCH_ABS_SAFE, input_result.get(), input_expected.get(), elements, 1e-6));
        }

        AND_THEN("invert = false") {
            test::randomize(input_expected.get(), elements, randomizer);
            test::copy(input_expected.get(), input_result.get(), elements);

            cuda::signal::cylinder(input_result.share(), strides, input_result.share(), strides, shape,
                                   center, radius, length, taper, math::inverse(fwd_transform), false, stream);
            for (size_t idx = 0; idx < elements; ++idx)
                input_expected[idx] *= invert ? 1 - mask_expected[idx] : mask_expected[idx];

            stream.synchronize();
            REQUIRE(test::Matcher(test::MATCH_ABS_SAFE, input_result.get(), input_expected.get(), elements, 1e-6));
        }
    }
}

TEMPLATE_TEST_CASE("cuda::signal::cylinder(), vs cpu", "[assets][noa][cuda]",
                   float, double, cfloat_t, cdouble_t) {
    const uint ndim = GENERATE(2u, 3u);
    const dim4_t shape = test::getRandomShapeBatched(ndim);
    const dim4_t strides = shape.strides();
    const dim_t elements = shape.elements();

    cuda::Stream gpu_stream;
    cpu::Stream cpu_stream;
    cuda::memory::PtrManaged<TestType> input(elements, gpu_stream);
    cuda::memory::PtrManaged<TestType> output_cpu(elements, gpu_stream);
    cuda::memory::PtrManaged<TestType> output_cuda(elements, gpu_stream);

    test::Randomizer<float> randomizer(-5, 5);
    test::randomize(input.get(), elements, randomizer);

    const auto center = float3_t(dim3_t(shape.get(1)) / 2) + randomizer.get();
    const auto radius = math::abs(randomizer.get() * 10);
    const auto length = math::abs(randomizer.get() * 10);
    const float edge_size = math::abs(randomizer.get());
    const bool invert = test::Randomizer<int>(0, 1).get();
    const auto fwd_transform = geometry::euler2matrix(float3_t{randomizer.get(), randomizer.get(), randomizer.get()});

    cpu::signal::cylinder(input.share(), strides, output_cpu.share(), strides, shape,
                          center, radius, length, edge_size, math::inverse(fwd_transform), invert, cpu_stream);
    cpu_stream.synchronize();
    cuda::signal::cylinder(input.share(), strides, output_cuda.share(), strides, shape,
                           center, radius, length, edge_size, math::inverse(fwd_transform), invert, gpu_stream);
    gpu_stream.synchronize();
    REQUIRE(test::Matcher(test::MATCH_ABS_SAFE, output_cpu.get(), output_cuda.get(), elements, 1e-4));
}
