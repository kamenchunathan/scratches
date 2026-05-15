#pragma once

#include "common.hpp"

#if HOPPER_LOGGING

    #include "spdlog/spdlog.h"

    #ifndef SPDLOG_ACTIVE_LEVEL
        #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
    #endif

    #define HOPPER_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
    #define HOPPER_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
    #define HOPPER_INFO(...) SPDLOG_INFO(__VA_ARGS__)
    #define HOPPER_WARN(...) SPDLOG_WARN(__VA_ARGS__)
    #define HOPPER_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
    #define HOPPER_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

#else

    #define HOPPER_TRACE(...) ((void)0)
    #define HOPPER_DEBUG(...) ((void)0)
    #define HOPPER_INFO(...) ((void)0)
    #define HOPPER_WARN(...) ((void)0)
    #define HOPPER_ERROR(...) ((void)0)
    #define HOPPER_CRITICAL(...) ((void)0)

#endif
