#include "log/layer.hpp"

#include "common.hpp"

#include <spdlog/spdlog.h>

#if PLATFORM_WASM
    #include "log/console_sink.hpp"
#else
    #include <filesystem>
    #include <spdlog/sinks/daily_file_sink.h>
#endif

namespace logging {

auto LogLayer::build(std::shared_ptr<core::Application>) -> void {
#if HOPPER_LOGGING
    #if PLATFORM_WASM

    auto console_sink = std::make_shared<logging::BrowserConsoleSink>();
    console_sink->set_level(spdlog::level::trace);

    auto logger = std::make_shared<spdlog::logger>("hopper", console_sink);
    spdlog::set_default_logger(logger);

    #else

    std::filesystem::create_directories("logs");

    auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/hopper.log", 0, 0);
    file_sink->set_level(spdlog::level::trace);

    auto logger = std::make_shared<spdlog::logger>("hopper", file_sink);
    spdlog::set_default_logger(logger);

    #endif

#endif
}

} // namespace logging
