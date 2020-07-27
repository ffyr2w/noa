/**
 * @file Log.h
 * @brief Logging system used throughout the core and programs.
 * @author Thomas - ffyr2w
 * @date 25 Jul 2020
 */
#pragma once

#include <spdlog/spdlog.h>
#include "spdlog/fmt/fmt.h"
#include <spdlog/fmt/ostr.h>
#include "spdlog/fmt/bundled/ranges.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>


namespace Noa {

    class Log {
    public:
        static void Init() {
            std::vector<spdlog::sink_ptr> log_sinks;
            log_sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
            log_sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("noa.log"));

            log_sinks[0]->set_pattern("%^[%T] %n: %v%$");
            log_sinks[1]->set_pattern("[%T] [%l]: %v");

            s_core_logger = std::make_shared<spdlog::logger>("NOA",
                                                             begin(log_sinks),
                                                             end(log_sinks));
            spdlog::register_logger(s_core_logger);
            s_core_logger->set_level(spdlog::level::trace);
            s_core_logger->flush_on(spdlog::level::err);

            s_app_logger = std::make_shared<spdlog::logger>("APP",
                                                            begin(log_sinks),
                                                            end(log_sinks));
            spdlog::register_logger(s_app_logger);
            s_app_logger->set_level(spdlog::level::trace);
            s_app_logger->flush_on(spdlog::level::err);
        }

        inline static std::shared_ptr<spdlog::logger>& getCoreLogger() { return s_core_logger; }

        inline static std::shared_ptr<spdlog::logger>& getAppLogger() { return s_app_logger; }

    private:
        static std::shared_ptr<spdlog::logger> s_core_logger;
        static std::shared_ptr<spdlog::logger> s_app_logger;
    };

    // Initialize the loggers.
    std::shared_ptr<spdlog::logger> Noa::Log::s_core_logger;
    std::shared_ptr<spdlog::logger> Noa::Log::s_app_logger;

    // Class used as custom exception catch in main().
    class ReturnMain : public std::exception {
    };
}

// Core log macros
#define NOA_CORE_DEBUG(...)    Noa::Log::getCoreLogger()->debug(__VA_ARGS__)
#define NOA_CORE_TRACE(...)    Noa::Log::getCoreLogger()->trace(__VA_ARGS__)
#define NOA_CORE_INFO(...)     Noa::Log::getCoreLogger()->info(__VA_ARGS__)
#define NOA_CORE_WARN(...)     Noa::Log::getCoreLogger()->warn(__VA_ARGS__)
#define NOA_CORE_ERROR(...)    Noa::Log::getCoreLogger()->error(__VA_ARGS__); throw Noa::ReturnMain()

// App log macros
#define NOA_DEBUG(...)    Noa::Log::getAppLogger()->debug(__VA_ARGS__)
#define NOA_TRACE(...)    Noa::Log::getAppLogger()->trace(__VA_ARGS__)
#define NOA_INFO(...)     Noa::Log::getAppLogger()->info(__VA_ARGS__)
#define NOA_WARN(...)     Noa::Log::getAppLogger()->warn(__VA_ARGS__)
#define NOA_ERROR(...)    Noa::Log::getAppLogger()->error(__VA_ARGS__); throw Noa::ReturnMain()
