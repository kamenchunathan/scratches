#include "util/text.hpp"

namespace util {

std::string to_utf8(char32_t codepoint) {
    std::string utf8_string;

    if (codepoint < 0x80) {
        // 1-byte sequence (0xxxxxxx) - ASCII
        utf8_string += static_cast<char>(codepoint);
    } else if (codepoint < 0x800) {
        // 2-byte sequence (110xxxxx 10xxxxxx)
        utf8_string += static_cast<char>(0xC0 | (codepoint >> 6));
        utf8_string += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
        // Check for surrogate pairs (0xD800 to 0xDFFF) which are invalid Unicode codepoints
        if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
            // Invalid Unicode codepoint (surrogate pair)
            return "?"; // Return replacement character
        }
        utf8_string += static_cast<char>(0xE0 | (codepoint >> 12));
        utf8_string += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        utf8_string += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x110000) {
        // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        utf8_string += static_cast<char>(0xF0 | (codepoint >> 18));
        utf8_string += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        utf8_string += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        utf8_string += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        // Invalid Unicode codepoint (outside valid range 0x0 to 0x10FFFF)
        // Return replacement character
        return "?";
    }

    return utf8_string;
}

} // namespace util
