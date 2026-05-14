#include "common.hpp"

#if PLATFORM_WASM

    #include <emscripten.h>
    #include <emscripten/html5.h>

    #include "application.hpp"
    #include "web/layer.hpp"

EM_JS(void, my_log, (const char* msg), { console.log('BrowserLayer: ' + UTF8ToString(msg)); });

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
        my_log(e.what());
        return false;
    }
}
}

namespace web {

void run(std::shared_ptr<core::Application> app) {
    my_log("Hello world");
    auto opt_state = app->world.get_resource<web::BrowserLayer::State>();
    if (!opt_state) {
        my_log("State not found");
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
