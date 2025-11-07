#pragma once

#include <concepts>
#include <string>

// GENERATE_TYPE_ERASURE
template <typename T>
concept Printable = requires(T t) {
    { t.print() } -> std::same_as<void>;
    { t.toString() } -> std::convertible_to<std::string>;
};