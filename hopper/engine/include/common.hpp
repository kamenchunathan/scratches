#pragma once

#include "version.hpp"

#ifdef __EMSCRIPTEN__

    #define PLATFORM_WASM 1

#elif defined(_WIN32)

    #define PLATFORM_WINDOWS 1

#elif defined(__linux__)

    #define PLATFORM_LINUX 1

#endif

#if defined(__GNUC__) || defined(__clang__)
    #define HOPPER_ALWAYS_INLINE [[gnu::always_inline]] inline
#elif defined(_MSC_VER)
    #define HOPPER_ALWAYS_INLINE __forceinline
#else
    #define HOPPER_ALWAYS_INLINE inline
#endif
