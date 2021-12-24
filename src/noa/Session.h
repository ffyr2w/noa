/// \file noa/Session.h
/// \brief The base session.
/// \author Thomas - ffyr2w
/// \date 18/06/2021

#pragma once

#include "noa/Version.h"
#include "noa/common/Logger.h"
#include "noa/common/Profiler.h"
#include "noa/common/string/Format.h"

namespace noa {
    /// Creates and holds the static data necessary to run noa.
    /// There should only be one session at a given time.
    class Session {
    public:
        /// Creates a new sessions.
        /// \param name                 Name of the session.
        /// \param filename             Filename of the sessions log file.
        ///                             If it is an empty string, the logger only logs in the console.
        /// \param verbosity_console    Verbosity of the console. The log file, if any, is always set
        ///                             to the maximum verbosity.
        /// \param threads              The maximum number of threads used during a session. See Session::threads().
        /// \note The logger is always accessible. As such, its settings and its sinks can be replaced at any time.
        NOA_HOST Session(std::string_view name,
                         std::string_view filename,
                         Logger::Level verbosity_console,
                         size_t threads) {
            logger = Logger(name, filename, verbosity_console);
            NOA_PROFILE_BEGIN_SESSION(string::format("{}_profiler.json", name));
            Session::threads(threads);
        }

        /// Creates a new session.
        NOA_HOST Session(std::string_view name, std::string_view filename, Logger::Level verbosity_console) {
            *this = Session(name, filename, verbosity_console, 0); // max threads
        }

        /// Creates a new session.
        NOA_HOST explicit Session(std::string_view name) {
            std::string filename(name);
            filename += "log";
            *this = Session(name, filename, Logger::BASIC, 0); // max threads
        }

        /// Sets the maximum number of internal threads used by a session.
        /// \param threads  Maximum number of threads.
        ///                 If 0, retrieve value from environmental variable NOA_THREADS or OMP_NUM_THREADS.
        ///                 If these variables are empty or not defined, try to deduce the number of available
        ///                 threads and use this number instead.
        /// \note This is the maximum number of internal threads. Users can of course create additional threads
        ///       using tools from the library, e.g. ThreadPool or Stream.
        NOA_HOST static void threads(size_t threads);

        /// Returns the maximum number of internal threads.
        [[nodiscard]] NOA_HOST static size_t threads() noexcept {
            return m_threads;
        }

        /// Unwind all the nested exceptions that were thrown and caught during this session.
        /// \note This function is meant to be called from the catch scope of main() before exiting the program.
        /// \note It is meant to be called using the default values: `session.backtrace()`.
        NOA_HOST void backtrace(const std::exception_ptr& exception_ptr = std::current_exception(), size_t level = 0);

        NOA_HOST ~Session() {
            NOA_PROFILE_END_SESSION();
        }

    public:
        NOA_HOST static std::string version() { return NOA_VERSION; }
        NOA_HOST static std::string url() { return NOA_URL; }

    public:
        /// Logger used by all functions in the library.
        static Logger logger;

    private:
        static size_t m_threads;
    };
}
