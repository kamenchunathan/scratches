#pragma once

#include <cstdint>
#include <expected>
#include <vector>

#include "input.hpp"

/* Parses input text from the terminal to  input events
 * Eventually may be configurable with multiple input modes and formats but currently
 * only works with SGR_Extended as legacy modes are difficult
 */
class InputParser {
public:
    enum class ParseError {
        Incomplete,
        Malformed,
    };

    InputParser(std::string_view input): input_(input) {}
    std::vector<core::input::Event> parse();

private:
    enum class Mode { Normal, Escape };

    std::string_view input_;
    std::vector<core::input::Event> input_events_;
    Mode mode_ = Mode::Normal;
    std::uint32_t cursor_ = 0;

    void parse_char();
    std::expected<void, ParseError> parse_escape_code();
    void advance(uint32_t n = 1);
};
