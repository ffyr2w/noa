/// \file noa/Assert.h
/// \brief Assertions.
/// \author Thomas - ffyr2w
/// \date 20 Jul 2020

#pragma once

#include "noa/Definitions.h"

// Ignore all assertions for CUDA device code.
#if defined(NOA_ENABLE_ASSERTS) && !defined(__CUDA_ARCH__)

#include <string>
#include <filesystem>
#include "noa/Session.h"

namespace noa::details {
    NOA_IH void logAssert(const char* file, const char* function, int line) {
        namespace fs = std::filesystem;
        size_t idx = std::string(file).rfind(std::string("noa") + fs::path::preferred_separator);
        noa::Session::logger.error("{}:{}:{}: Assertion failed.",
                                   idx == std::string::npos ? fs::path(file).filename().string() : file + idx,
                                   function, line);
    }
}

#define NOA_ASSERT(check) do { if(!(check)) { noa::details::logAssert(__FILE__, __FUNCTION__, __LINE__); NOA_DEBUG_BREAK(); } } while (false)

#else
    #define NOA_ASSERT(check)
#endif
