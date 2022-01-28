#include <noa/common/io/ImageFile.h>
#include <noa/cpu/memory/PtrHost.h>
#include <noa/cpu/filter/Median.h>

#include "Helpers.h"
#include "Assets.h"
#include <catch2/catch.hpp>

TEST_CASE("cpu::filter::median()", "[assets][noa][cpu][filter]") {
    using namespace noa;

    const path_t path_base = test::PATH_NOA_DATA / "filter";
    YAML::Node tests = YAML::LoadFile(path_base / "tests.yaml")["median"]["tests"];
    io::ImageFile file;
    cpu::Stream stream;

    for (size_t nb = 0; nb < tests.size(); ++nb) {
        INFO("test number = " << nb);

        const YAML::Node& test = tests[nb];
        const auto filename_input = path_base / test["input"].as<path_t>();
        const auto window = test["window"].as<size_t>();
        const auto dim = test["dim"].as<int>();
        const auto border = test["border"].as<BorderMode>();
        const auto filename_expected = path_base / test["expected"].as<path_t>();

        file.open(filename_input, io::READ);
        const size4_t shape = file.shape();
        const size4_t stride = shape.strides();
        const size_t elements = shape.elements();
        cpu::memory::PtrHost<float> input(elements);
        file.readAll(input.get());

        cpu::memory::PtrHost<float> expected(elements);
        file.open(filename_expected, io::READ);
        file.readAll(expected.get());

        cpu::memory::PtrHost<float> result(elements);
        if (dim == 1)
            cpu::filter::median1(input.get(), stride, result.get(), stride, shape, border, window, stream);
        else if (dim == 2)
            cpu::filter::median2(input.get(), stride, result.get(), stride, shape, border, window, stream);
        else if (dim == 3)
            cpu::filter::median3(input.get(), stride, result.get(), stride, shape, border, window, stream);
        else
            FAIL("dim is not correct");

        REQUIRE(test::Matcher(test::MATCH_ABS, result.get(), expected.get(), result.size(), 1e-5));
    }
}
