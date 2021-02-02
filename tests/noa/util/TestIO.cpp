#include "../../Helpers.h"
#include "noa/util/IO.h"

#include <catch2/catch.hpp>

using namespace ::Noa;

TEST_CASE("IO: read and write", "[noa][IO]") {
    fs::path test_dir = "testIO";
    fs::path test_file = test_dir / "test_write_float1.txt";
    fs::create_directory(test_dir);

    auto random_size = GENERATE(take(3, random(134, 4000)));
    auto elements = static_cast<size_t>(random_size);
    IO::DataType dtype = GENERATE(IO::DataType::byte, IO::DataType::ubyte,
                                  IO::DataType::int16, IO::DataType::uint16,
                                  IO::DataType::int32, IO::DataType::uint32,
                                  IO::DataType::float32, IO::DataType::cfloat32,
                                  IO::DataType::cint16);
    size_t bytes_per_elements = IO::bytesPerElement(dtype);
    bool batch = GENERATE(true, false);
    bool swap = GENERATE(true, false);

    INFO("size: " << elements << ", dtype: " << IO::toString(dtype) << ", batch: " << batch << ", swap: " << swap);

    AND_THEN("complex != real dtype") {
        if (!IO::isComplex(dtype)) {
            std::unique_ptr<cfloat_t[]> data = std::make_unique<cfloat_t[]>(elements);
            std::fstream file(test_file, std::ios::out | std::ios::trunc);
            Errno err = IO::writeComplexFloat<2048>(data.get(), file, elements, dtype, batch, swap);
            REQUIRE(err == Errno::dtype_real);
            file.close();
        } else {
            std::unique_ptr<float[]> data = std::make_unique<float[]>(elements);
            std::fstream file(test_file, std::ios::out | std::ios::trunc);
            Errno err = IO::writeFloat<2048>(data.get(), file, elements, dtype, batch, swap);
            REQUIRE(err == Errno::dtype_complex);
            file.close();
        }
    }

    AND_WHEN("write and read") {
        if (IO::isComplex(dtype)) {
            std::unique_ptr<cfloat_t[]> data = std::make_unique<cfloat_t[]>(elements);
            std::unique_ptr<cfloat_t[]> read_data = std::make_unique<cfloat_t[]>(elements);
            auto* data_as_float = reinterpret_cast<float*>(data.get());
            auto* read_data_as_float = reinterpret_cast<float*>(read_data.get());

            Test::IntRandomizer<int> randomizer(0, 127);
            for (size_t i{0}; i < elements * 2; ++i)
                data_as_float[i] = static_cast<float>(randomizer.get());

            std::fstream file(test_file, std::ios::out | std::ios::trunc);
            Errno err = IO::writeComplexFloat<2048>(data.get(), file, elements, dtype, batch, swap);
            REQUIRE_ERRNO_GOOD(err);
            file.close();

            REQUIRE(fs::file_size(test_file) == elements * bytes_per_elements);

            // read back the array
            file.open(test_file, std::ios::in);
            err = IO::readComplexFloat<2048>(file, read_data.get(), elements, dtype, batch, swap);
            REQUIRE_ERRNO_GOOD(err);

            float diff = Test::getDifference(data_as_float, read_data_as_float, elements * 2);
            REQUIRE_THAT(diff, Catch::WithinULP(0.f, 2));

        } else {
            std::unique_ptr<float[]> data = std::make_unique<float[]>(elements);
            std::unique_ptr<float[]> read_data = std::make_unique<float[]>(elements);

            Test::IntRandomizer<int> randomizer(0, 127);
            for (size_t i{0}; i < elements; ++i)
                data[i] = static_cast<float>(randomizer.get());

            std::fstream file(test_file, std::ios::out | std::ios::trunc);
            Errno err = IO::writeFloat<2048>(data.get(), file, elements, dtype, batch, swap);
            REQUIRE_ERRNO_GOOD(err);
            file.close();

            REQUIRE(fs::file_size(test_file) == elements * bytes_per_elements);

            // read back the array
            file.open(test_file, std::ios::in);
            err = IO::readFloat<2048>(file, read_data.get(), elements, dtype, batch, swap);
            REQUIRE_ERRNO_GOOD(err);

            float diff = Test::getDifference(data.get(), read_data.get(), elements);
            REQUIRE_THAT(diff, Catch::WithinULP(0.f, 2));
        }
    }
    fs::remove_all("testIO");
}

TEST_CASE("IO: swapEndian", "[noa][IO]") {
    std::unique_ptr<float[]> data1 = std::make_unique<float[]>(100);
    std::unique_ptr<float[]> data2 = std::make_unique<float[]>(100);
    for (size_t i{0}; i < 100; ++i) {
        auto t = static_cast<float>(Test::pseudoRandom(-1234434, 94321458));
        data1[i] = t;
        data2[i] = t;
    }
    size_t dtype = IO::bytesPerElement(IO::DataType::float32);
    Errno err = IO::swapEndian(reinterpret_cast<char*>(data1.get()), 100, dtype);
    REQUIRE_ERRNO_GOOD(err);
    err = IO::swapEndian(reinterpret_cast<char*>(data1.get()), 100, dtype);
    REQUIRE_ERRNO_GOOD(err);
    float diff{0};
    for (size_t i{0}; i < 100; ++i)
        diff += data1[i] - data2[i];
    REQUIRE_THAT(diff, Catch::WithinULP(0.f, 2));
}

TEST_CASE("IO: dtype is float", "[noa][IO]") {
    fs::path test_dir = "testIO";
    fs::path test_file = test_dir / "test_write_float1.txt";
    fs::create_directory(test_dir);

    auto random_size = GENERATE(take(3, random(134, 4000)));
    auto elements = static_cast<size_t>(random_size);
    IO::DataType dtype = IO::DataType::float32;
    size_t bytes_per_elements = IO::bytesPerElement(dtype);
    bool batch = GENERATE(true, false);
    bool swap = GENERATE(true, false);

    INFO("size: " << elements << ", dtype: " << IO::toString(dtype) << ", batch: " << batch << ", swap: " << swap);

    std::unique_ptr<float[]> data = std::make_unique<float[]>(elements);
    std::unique_ptr<float[]> read_data = std::make_unique<float[]>(elements);

    Test::RealRandomizer<float> randomizer(-5, 5);
    for (size_t i{0}; i < elements; ++i)
        data[i] = randomizer.get();

    std::fstream file(test_file, std::ios::out | std::ios::trunc);
    Errno err = IO::writeFloat<2048>(data.get(), file, elements, dtype, batch, swap);
    REQUIRE_ERRNO_GOOD(err);
    file.close();

    REQUIRE(fs::file_size(test_file) == elements * bytes_per_elements);

    // read back the array
    file.open(test_file, std::ios::in);
    err = IO::readFloat<2048>(file, read_data.get(), elements, dtype, batch, swap);
    REQUIRE_ERRNO_GOOD(err);

    float diff = Test::getDifference(data.get(), read_data.get(), elements);
    REQUIRE_THAT(diff, Catch::WithinULP(0.f, 2));
}
