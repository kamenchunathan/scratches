#pragma once

#include "common.hpp"

#if PLATFORM_WASM

    #include <emscripten.h>

    #include <memory>
    #include <string>

    #include <spdlog/common.h>
    #include <spdlog/pattern_formatter.h>
    #include <spdlog/sinks/sink.h>

EM_JS(void, _hopper_console_log, (int level, const char* msg), {
    switch (level) {
        case 0:
        case 1:
            console.debug(UTF8ToString(msg));
            break;
        case 2:
            console.log(UTF8ToString(msg));
            break;
        case 3:
            console.warn(UTF8ToString(msg));
            break;
        case 4:
        case 5:
            console.error(UTF8ToString(msg));
            break;
        default:
            console.log(UTF8ToString(msg));
    }
});

namespace logging {

class BrowserConsoleSink: public spdlog::sinks::sink {
public:
    BrowserConsoleSink() {
        formatter_ = std::make_unique<spdlog::pattern_formatter>();
    }

    void log(const spdlog::details::log_msg& msg) override {
        if (!should_log(msg.level))
            return;

        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        formatted.push_back('\0');

        _hopper_console_log(static_cast<int>(msg.level), formatted.data());
    }

    void flush() override {}

    void set_pattern(const std::string& pattern) override {
        formatter_ = std::make_unique<spdlog::pattern_formatter>(pattern);
    }

    void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override {
        formatter_ = std::move(sink_formatter);
    }

private:
    std::unique_ptr<spdlog::formatter> formatter_;
};

} // namespace logging

#endif
