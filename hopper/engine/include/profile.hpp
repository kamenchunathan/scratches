#pragma once

#ifdef HOPPER_TRACY
    #include <tracy/Tracy.hpp>

    #define HOPPER_ZONE() ZoneScoped
    #define HOPPER_ZONE_NAMED(label) ZoneScopedN(label)
    #define HOPPER_ZONE_DYNAMIC(str, len) \
        ZoneScoped; \
        ZoneName((str), (len))
    #define HOPPER_FRAME_MARK() FrameMark
    #define HOPPER_FRAME_MARK_NAMED(name) FrameMarkNamed(name)
    #define HOPPER_PLOT(name, val) TracyPlot(name, static_cast<double>(val))
    #define HOPPER_ALLOC(ptr, size) TracyAlloc((ptr), (size))
    #define HOPPER_FREE(ptr) TracyFree(ptr)
    #define HOPPER_MESSAGE(literal) TracyMessage(literal, sizeof(literal) - 1)

#else // !HOPPER_TRACY ---------------------------------------------------------

    #define HOPPER_ZONE()
    #define HOPPER_ZONE_NAMED(label)
    #define HOPPER_ZONE_DYNAMIC(str, len)
    #define HOPPER_FRAME_MARK()
    #define HOPPER_FRAME_MARK_NAMED(name)
    #define HOPPER_PLOT(name, val)
    #define HOPPER_ALLOC(ptr, size)
    #define HOPPER_FREE(ptr)
    #define HOPPER_MESSAGE(literal)

#endif // HOPPER_TRACY
