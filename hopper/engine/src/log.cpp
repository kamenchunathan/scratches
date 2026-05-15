#include "log.hpp"

#if HOPPER_LOGGING

    #include <filesystem>
    #include <mutex>

    #include <spdlog/async.h>
    #include <spdlog/sinks/base_sink.h>
    #include <spdlog/sinks/rotating_file_sink.h>
    #include <spdlog/sinks/stdout_color_sinks.h>

    #if PLATFORM_WASM
        #include <emscripten.h>

        #include <memory>
        #include <string>

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

class BrowserConsoleSink final: public spdlog::sinks::sink {
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
    #endif

    // When both logging and profiling are active, route every formatted log line
    // into the Tracy message log so the profiling timeline includes log context.
    #if HOPPER_TRACY

        #include <tracy/Tracy.hpp>

class TracyMessageSink final: public spdlog::sinks::base_sink<std::mutex> {
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t buf;
        this->formatter_->format(msg, buf);
        // memory_buf_t is std::string when SPDLOG_USE_STD_FORMAT is defined
        TracyMessage(buf.data(), buf.size());
    }

    void flush_() override {}
};
    #endif // HOPPER_TRACY

namespace {

// Pre-registered subsystem names. Any unknown category passed to log::get()
// falls back to the default "engine" logger, so registration here is only an
// optimisation — it is not required for a category to work.
constexpr std::string_view k_subsystems[] = {
    "renderer",
    "ecs",
    "input",
    "terminal",
    "physics",
};

std::once_flag g_init_flag;

std::shared_ptr<spdlog::logger> make_logger(
    std::string_view name,
    const std::vector<spdlog::sink_ptr>& sinks,
    bool async,
    const std::string& pattern
) {
    std::shared_ptr<spdlog::logger> logger;
    if (async) {
        logger = std::make_shared<spdlog::async_logger>(
            std::string(name),
            sinks.begin(),
            sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block
        );
    } else {
        logger = std::make_shared<spdlog::logger>(std::string(name), sinks.begin(), sinks.end());
    }
    // Runtime level set to trace; compile-time filtering via SPDLOG_ACTIVE_LEVEL
    // strips call sites for levels below the threshold at compile time.
    logger->set_level(spdlog::level::trace);
    logger->set_pattern(pattern);
    return logger;
}

} // namespace

namespace logging {

void init(const Config& cfg) {
    std::call_once(g_init_flag, [&cfg] {
        std::vector<spdlog::sink_ptr> sinks;

    #if PLATFORM_WASM

        auto console_sink = std::make_shared<BrowserConsoleSink>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
        sinks.push_back(std::move(console_sink));

        #ifdef NDEBUG
                // TODO(P2): Add RemoteLogSink — fire-and-forget fetch() POST to /api/logs
                // for server-side log aggregation.  Debug builds log only to the browser
                // console; release builds additionally forward warnings and errors to the
                // server endpoint.
        #endif

        // WASM has no filesystem, no Tracy integration, and no async threading.
        // Async is forcibly disabled regardless of cfg.async.
        bool const use_async = false;

    #else

        // Rotating file sink — always included when HOPPER_LOGGING is on.
        try {
            std::filesystem::create_directories(cfg.log_dir);
            auto log_path  = std::filesystem::path(cfg.log_dir) / "engine.log";
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_path.string(),
                cfg.max_file_mb * 1024UZ * 1024UZ,
                cfg.max_files
            );
            file_sink->set_level(spdlog::level::trace);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
            sinks.push_back(std::move(file_sink));
        } catch (const std::exception& e) {
            // TODO: Do something here
        }

        #if HOPPER_TRACY
        
        auto tracy_sink = std::make_shared<TracyMessageSink>();
        // Omit timestamp from Tracy messages — the profiler adds its own.
        tracy_sink->set_pattern("[%n] [%l] %v");
        sinks.push_back(std::move(tracy_sink));

        #endif // HOPPER_TRACY

        if (cfg.async) {
            spdlog::init_thread_pool(8192, 1);
        }

        bool const use_async = cfg.async;

    #endif

        const std::string pattern = "[%H:%M:%S.%e] [%n] [%^%l%$] %v";

        auto default_logger = make_logger("app", sinks, use_async, pattern);
        spdlog::set_default_logger(default_logger);

        for (std::string_view name: k_subsystems) {
            spdlog::register_logger(make_logger(name, sinks, use_async, pattern));
        }

        spdlog::info("Logger initialised (dir={}, async={})", cfg.log_dir, use_async);
    });
}

void shutdown() {
    spdlog::shutdown();
}

spdlog::logger* get(const char* category) {
    // spdlog::get acquires a shared_lock on the registry; the returned shared_ptr
    // keeps the logger alive for the duration of the expression, so the raw
    // pointer is valid for the call. The registry itself outlives the process.
    if (auto logger = spdlog::get(category)) {
        return logger.get();
    }
    return spdlog::default_logger_raw();
}

} // namespace logging

#endif // HOPPER_LOGGING
