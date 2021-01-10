/**
 * @file Base.h
 * @brief Contain the minimum files to include for the core.
 * @author Thomas - ffyr2w
 * @date 20 Jul 2020
 */
#pragma once

// These header are mostly included for the precompiled header.
// In the code base, it is good practice to explicitly include the necessary std headers,
// even if they are included in this file.

// Streams:
#include <iostream>
#include <fstream>
#include <string>
#include <string_view>

// Containers:
#include <map>
#include <unordered_map>
#include <vector>
#include <array>
#include <tuple>

// Others:
#include <cstdint>
#include <cctype>
#include <cstring>  // std::strerror
#include <cerrno>   // errno
#include <cmath>

#include <filesystem>
#include <thread>
#include <utility>
#include <algorithm>
#include <memory>
#include <type_traits>
#include <complex>

// NOA_API and NOA_VERSION*
#include "noa/API.h"
#include "noa/Version.h"

namespace Noa {
    namespace fs = std::filesystem;

    /** Memory resource used by allocators. */
    enum class Resource : uint8_t {
        host,
        pinned,
        device
    };


    /** To which intent the file/pointer should be used. Often used as Flag<Intent>. */
    enum class Intent {
        read = 0x01,
        write = 0x02,
        trunc = 0x04,
        app = 0x08,
        ate = 0x08,
        bin = 0x10
    };
}

// NOA base:
#include "noa/util/Flag.h"
#include "noa/util/Errno.h"
#include "noa/util/Log.h"
#include "noa/util/Exception.h"


// Logging macros. They are macros, mostly to keep some uniformity with NOA_LOG_ERROR, which
// needs to be a macro because of __FILE__, __FUNCTION__ and __LINE__.
#if NOA_DEBUG
#define NOA_LOG_DEBUG(...) ::Noa::Log::get()->debug(__VA_ARGS__)
#else
#define NOA_LOG_DEBUG
#endif

#define NOA_LOG_TRACE(...) Noa::Log::get()->trace(__VA_ARGS__)
#define NOA_LOG_INFO(...)  Noa::Log::get()->info(__VA_ARGS__)
#define NOA_LOG_WARN(...)  Noa::Log::get()->warn(__VA_ARGS__)
#define NOA_LOG_ERROR(...) throw Noa::Error(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define NOA_LOG_ERROR_FUNC(func, ...) throw Noa::Error(__FILE__, func, __LINE__, __VA_ARGS__)
