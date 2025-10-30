#include "common.hpp"

// #if PLATFORM_WASM

#include <emscripten.h>
#include <emscripten/html5.h>

#include "application.hpp"
#include "web/layer.hpp"

EM_JS(void, my_log, (const char* msg), { console.log('BrowserLayer: ' + UTF8ToString(msg)); });

extern "C" {

EMSCRIPTEN_KEEPALIVE
bool main_loop_callback(double time, void* user_data) {
    try {
        BrowserLayer::State* state = static_cast<BrowserLayer::State*>(user_data);
        double delta_ms = time - state->last_time;
        state->last_time = time;

        state->app->tick(delta_ms);
        return !state->app->should_exit();
    } catch (const std::exception& e) {
        my_log(e.what());
        return false;
    }
}
}

void BrowserLayer::run(std::shared_ptr<core::Application> app) {
    state_ = State {app, 0};
    emscripten_request_animation_frame_loop(main_loop_callback, &state_);
}

void BrowserLayer::build(std::shared_ptr<core::Application> app) {
    app->set_runner([this](std::shared_ptr<core::Application> a) { run(a); });
}

// #endif
