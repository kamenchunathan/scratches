#pragma once

// HOPPER_VERSION_MAJOR, MINOR, and PATCH are provided as compiler defines
// via meson.build to ensure the LSP always finds this header while
// keeping the version synchronized with the build system.

// HOPPER_SINCE(major, minor)
//
// Decorates a declaration to document when it was introduced.
// Informational only — no compiler effect.
#define HOPPER_SINCE(major, minor)

// HOPPER_DEPRECATED(major, minor, replacement)
//
// Marks a declaration as deprecated as of the given version.
// Emits a compiler warning (-Wdeprecated-declarations) at every call site.
#define HOPPER_DEPRECATED(major, minor, replacement) \
    [[deprecated("Deprecated since " #major "." #minor " — use " replacement " instead")]]

// HOPPER_DEPRECATED_TYPEDEF(major, minor, replacement)
//
// Same as HOPPER_DEPRECATED but for type aliases.
#define HOPPER_DEPRECATED_TYPEDEF(major, minor, replacement) \
    [[deprecated("Deprecated since " #major "." #minor " — use " replacement " instead")]]
