#pragma once

#include <string>

namespace util {
/* Converts a single Unicode codepoint (char32_t) to its UTF-8 string representation.
 * Returns a string containing the UTF-8 bytes.
 * For invalid codepoints, it returns a "?" replacement character.
 */
std::string to_utf8(char32_t codepoint);

} // namespace util
