#include <noa/cpu/masks/Sphere.h>

#include <noa/cpu/PtrHost.h>
#include <noa/io/files/MRCFile.h>

#include "Helpers.h"
#include <catch2/catch.hpp>

using namespace Noa;

// Just compare against manually checked data.
TEST_CASE("CPU::Mask - sphere", "[noa][cpu][masks]") {
    Test::Randomizer<float> randomizer(-5, 5);
    path_t path_data = Test::PATH_TEST_DATA / "masks";
    MRCFile file;

    size3_t shape;
    float3_t shifts;
    float radius;
    float taper;

    int test_number = GENERATE(1, 2, 3, 4, 5);
    if (test_number == 1) {
        shape = {128, 128, 1};
        shifts = {0, 0, 0};
        radius = 40;
        taper = 0;
        path_data /= "sphere_01.mrc";
    } else if (test_number == 2) {
        shape = {128, 128, 1};
        shifts = {0, 0, 0};
        radius = 41;
        taper = 0;
        path_data /= "sphere_02.mrc";
    } else if (test_number == 3) {
        shape = {256, 256, 1};
        shifts = {-127, 0, 0};
        radius = 108;
        taper = 19;
        path_data /= "sphere_03.mrc";
    } else if (test_number == 4) {
        shape = {100,100,100};
        shifts = {20,0,-20};
        radius = 30;
        taper = 0;
        path_data /= "sphere_04.mrc";
    } else if (test_number == 5) {
        shape = {100,100,100};
        shifts = {20,0,-20};
        radius = 20;
        taper = 10;
        path_data /= "sphere_05.mrc";
    }
    INFO("test number: " << test_number);

    size_t elements = getElements(shape);
    PtrHost<float> mask_expected(elements);
    file.open(path_data, IO::READ);
    file.readAll(mask_expected.get());

    PtrHost<float> input_expected(elements);
    PtrHost<float> input_result(elements);
    PtrHost<float> mask_result(elements);

    AND_THEN("invert = true") {
        Test::initDataRandom(input_expected.get(), elements, randomizer);
        std::memcpy(input_result.get(), input_expected.get(), elements * sizeof(float));

        // Test saving the mask.
        Mask::sphere<true>(mask_result.get(), shape, shifts, radius, taper);
        float diff = Test::getAverageDifference(mask_expected.get(), mask_result.get(), elements);
        REQUIRE_THAT(diff, Test::isWithinAbs(float(0.), 1e-7));

        // Test on-the-fly, in-place.
        Mask::sphere<true>(input_result.get(), input_result.get(), shape, shifts, radius, taper, 1);
        for (size_t idx = 0; idx < elements; ++idx)
            input_expected[idx] *= mask_expected[idx];
        diff = Test::getAverageDifference(input_result.get(), input_expected.get(), elements);
        REQUIRE_THAT(diff, Test::isWithinAbs(float(0.), 1e-7));
    }

    AND_THEN("invert = false") {
        for (size_t idx = 0; idx < elements; ++idx)
            mask_expected[idx] = 1 - mask_expected[idx]; // test data is invert=true
        Test::initDataRandom(input_expected.get(), elements, randomizer);
        std::memcpy(input_result.get(), input_expected.get(), elements * sizeof(float));

        // Test saving the mask. Default should be invert=false
        Mask::sphere(mask_result.get(), shape, shifts, radius, taper);
        float diff = Test::getAverageDifference(mask_expected.get(), mask_result.get(), elements);
        REQUIRE_THAT(diff, Test::isWithinAbs(float(0.), 1e-7));

        // Test on-the-fly, in-place.
        Mask::sphere(input_result.get(), input_result.get(), shape, shifts, radius, taper, 1);
        for (size_t idx = 0; idx < elements; ++idx)
            input_expected[idx] *= mask_expected[idx];
        diff = Test::getAverageDifference(input_result.get(), input_expected.get(), elements);
        REQUIRE_THAT(diff, Test::isWithinAbs(float(0.), 1e-7));
    }
}
