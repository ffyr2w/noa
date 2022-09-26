#include <noa/unified/Array.h>
#include <noa/unified/math/Random.h>
#include <noa/unified/memory/Copy.h>
#include <noa/unified/memory/Factory.h>
#include <noa/unified/memory/Permute.h>

#include <catch2/catch.hpp>

#include "Helpers.h"

using namespace ::noa;

TEMPLATE_TEST_CASE("unified::memory::permute", "[noa][unified]", int32_t, float, double, cfloat_t) {
    const std::array<dim4_t, 6> permutations{dim4_t{0, 1, 2, 3},
                                             dim4_t{0, 1, 3, 2},
                                             dim4_t{0, 3, 1, 2},
                                             dim4_t{0, 3, 2, 1},
                                             dim4_t{0, 2, 1, 3},
                                             dim4_t{0, 2, 3, 1}};
    const uint ndim = GENERATE(2U, 3U);
    const uint number = GENERATE(0U, 1U, 2U, 3U, 4U, 5U);
    const dim4_t permutation = permutations[number];
    const size4_t shape = test::getRandomShapeBatched(ndim);
    const size4_t permuted_shape = indexing::reorder(shape, permutation);
    if (ndim == 2 && !(all(permutation == dim4_t{0, 1, 2, 3}) || all(permutation == dim4_t{0, 1, 3, 2})))
        return; // while this is technically OK, it doesn't make much sense to test these...

    StreamGuard stream(Device{}, Stream::DEFAULT);
    Array<TestType> data = math::random<TestType>(math::uniform_t{}, shape, -5, 5);
    Array<TestType> expected(permuted_shape);

    cpu::memory::permute<TestType>(data.share(), data.strides(), data.shape(),
                                   expected.share(), expected.strides(), permutation, stream.cpu());

    {
        Array<TestType> result(permuted_shape);
        memory::permute(data, result, permutation);
        REQUIRE(test::Matcher(test::MATCH_ABS, expected, result, 1e-8));
    }

    {
        if (!Device::any(Device::GPU))
            return;
        Array<TestType> result(permuted_shape, ArrayOption{Device("gpu"), Allocator::MANAGED});
        memory::permute(data.to(result.device()), result, permutation);
        REQUIRE(test::Matcher(test::MATCH_ABS, expected, result, 1e-8));
    }
}

TEST_CASE("unified::memory::permute, broadcast", "[noa][unified]") {
    const std::array<dim4_t, 6> permutations{dim4_t{0, 1, 2, 3},
                                             dim4_t{0, 1, 3, 2},
                                             dim4_t{0, 3, 1, 2},
                                             dim4_t{0, 3, 2, 1},
                                             dim4_t{0, 2, 1, 3},
                                             dim4_t{0, 2, 3, 1}};
    const uint number = GENERATE(0U, 1U, 2U, 3U, 4U, 5U);
    const dim4_t permutation = permutations[number];
    const size4_t shape{1, 20, 50, 60};
    const size4_t permuted_shape = indexing::reorder(shape, permutation);

    std::vector<Device> devices = {Device("cpu")};
    if (Device::any(Device::GPU))
        devices.emplace_back("gpu");

    for (auto& device: devices) {
        INFO(device);
        StreamGuard stream(device, Stream::DEFAULT);
        ArrayOption options(device, Allocator::MANAGED);

        Array<float> result0(permuted_shape, options);
        Array<float> result1(permuted_shape, options);

        Array<float> data0 = memory::arange<float>({1, 1, 50, 60}, 0, 1, options);
        Array<float> data1({1, 20, 50, 60}, options);
        memory::copy(data0, data1);

        memory::permute(data0, result0, permutation);
        memory::permute(data1, result1, permutation);

        REQUIRE(test::Matcher(test::MATCH_ABS, result0, result1, 1e-8));
    }
}
