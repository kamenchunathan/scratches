#pragma once

#include "common.hpp"

#if HOPPER_LOGGING

    #include "spdlog/spdlog.h"

namespace logging {

struct Config {
    const std::string_view log_dir = "logs";
    std::size_t max_file_mb        = 5;
    std::size_t max_files          = 3;
    bool async                     = false; // Use async (lock-free ring buffer) logger
};

void init(const Config& config = {});
void shutdown();

/// Returns the named logger if registered, otherwise the default logger.
spdlog::logger* get(const char* category);

} // namespace logging

    #ifndef SPDLOG_ACTIVE_LEVEL
        #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
    #endif

    #define HOPPER_TRACE(cat, ...) SPDLOG_LOGGER_TRACE(::logging::get(cat), __VA_ARGS__)
    #define HOPPER_DEBUG(cat, ...) SPDLOG_LOGGER_DEBUG(::logging::get(cat), __VA_ARGS__)
    #define HOPPER_INFO(cat, ...) SPDLOG_LOGGER_INFO(::logging::get(cat), __VA_ARGS__)
    #define HOPPER_WARN(cat, ...) SPDLOG_LOGGER_WARN(::logging::get(cat), __VA_ARGS__)
    #define HOPPER_ERROR(cat, ...) SPDLOG_LOGGER_ERROR(::logging::get(cat), __VA_ARGS__)
    #define HOPPER_CRITICAL(cat, ...) SPDLOG_LOGGER_CRITICAL(::logging::get(cat), __VA_ARGS__)

#else

    #define HOPPER_TRACE(...) ((void)0)
    #define HOPPER_DEBUG(...) ((void)0)
    #define HOPPER_INFO(...) ((void)0)
    #define HOPPER_WARN(...) ((void)0)
    #define HOPPER_ERROR(...) ((void)0)
    #define HOPPER_CRITICAL(...) ((void)0)

#endif
