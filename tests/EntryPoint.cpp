// This is the entry point to ALL tests.

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#include <noa/Session.h>

namespace test {
    noa::path_t PATH_TEST_DATA;
}

// The first
int main(int argc, char* argv[]) {
    Catch::Session catch_session; // There must be exactly one instance

    const char* path = std::getenv("NOA_TEST_DATA");
    if (path == nullptr) {
        std::cerr << "The environmental variable \"NOA_TEST_DATA\" is empty. "
                     "Set it to the path of the noa-data repository and try again.\n";
        return EXIT_FAILURE;
    }
    test::PATH_TEST_DATA = path;
    test::PATH_TEST_DATA /= "assets";

    int returnCode = catch_session.applyCommandLine(argc, argv);
    if (returnCode != 0) // Indicates a command line error
        return returnCode;

    noa::Session noa_session("tests", "tests.log", noa::Logger::SILENT);
    int numFailed = catch_session.run();
    std::filesystem::remove("tests.log");
    return numFailed;
}
