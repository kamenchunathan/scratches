#pragma once

#include <cstdint>

#include "common.hpp"

#if defined(PLATFORM_LINUX) || defined(PLATFORM_WASM)
    #include <unistd.h>
#endif

#if defined(PLATFORM_LINUX)
    #include <sys/ioctl.h>
    #include <termios.h>
#elif defined(PLATFORM_WINDOWS)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <io.h>
    #include <windows.h>
    // windows.h defines TRANSPARENT and OPAQUE as GDI macros, conflicting with color.hpp constants
    #ifdef TRANSPARENT
        #undef TRANSPARENT
    #endif
    #ifdef OPAQUE
        #undef OPAQUE
    #endif
#endif

namespace term {

struct TerminalSize {
    std::uint32_t width;
    std::uint32_t height;
};

#if defined(PLATFORM_LINUX)

using NativeTerminalState = termios;

#elif defined(PLATFORM_WINDOWS)

struct NativeTerminalState {
    DWORD dwInputMode;
    DWORD dwOutputMode;
    UINT cpInput;
    UINT cpOutput;
};

#else

// WASM . Not intended to use the console through this api
struct NativeTerminalState {};

#endif

} // namespace term
