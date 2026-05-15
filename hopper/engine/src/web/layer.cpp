#include "common.hpp"

#if PLATFORM_WASM

    #include <emscripten.h>
    #include <emscripten/html5.h>

    #include <memory>

    #include "application.hpp"
    #include "log.hpp"
    #include "web/layer.hpp"

extern "C" {
EMSCRIPTEN_KEEPALIVE
bool main_loop_callback(double time, void* user_data) {
    try {
        web::BrowserLayer::State* state = static_cast<web::BrowserLayer::State*>(user_data);
        double delta_ms                 = time - state->last_time;
        state->last_time                = time;

        state->app->tick(delta_ms);
        return !state->app->should_exit();
    } catch (const std::exception& e) {
        HOPPER_ERROR("{}", e.what());
        return false;
    }
}
}

namespace web {

void run(std::shared_ptr<core::Application> app) {
    auto opt_state = app->world.get_resource<web::BrowserLayer::State>();
    if (!opt_state) {
        HOPPER_ERROR("State not found");
        return;
    }
    emscripten_request_animation_frame_loop(main_loop_callback, &opt_state->get());
}

void BrowserLayer::build(std::shared_ptr<core::Application> app) {
    app->world.insert_resource<web::BrowserLayer::State>(State {app, 0});
    app->set_runner([](std::shared_ptr<core::Application> a) { run(a); });
}

} // namespace web

#endif
