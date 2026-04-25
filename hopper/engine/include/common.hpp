#pragma once

#include "version.hpp"

#ifdef __EMSCRIPTEN__

    #define PLATFORM_WASM 1

#elif defined(_WIN32)

    #define PLATFORM_WINDOWS 1

#elif defined(__linux__)

    #define PLATFORM_LINUX 1

#endif
