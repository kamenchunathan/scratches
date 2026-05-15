#include <memory>

#include "application.hpp"
#include "ecs/system.hpp"
#include "ecs/world.hpp"
#include "input.hpp"
#include "log.hpp"
#include "log/layer.hpp"

#if PLATFORM_WASM
    #include "web/layer.hpp"
#else
    #include "term/layer.hpp"
#endif

void handle_input(ecs::World& world) {
    HOPPER_INFO("app", "hello world");
    if (auto inp_state = world.get_resource<core::input::InputState>(); inp_state.has_value()) {
        auto inp = inp_state->get();
        if (inp.just_pressed(core::input::KeyCode::Q)) {
            if (auto app = world.get_resource<core::Application*>(); app.has_value()) {
                app->get()->set_should_exit(true);
            }
        }
    }
}

int main() {
    static auto app = std::make_shared<core::Application>();
    app->world.insert_resource<core::Application*>(app.get());

#if PLATFORM_WASM
    web::BrowserLayer browser_layer {};
    app->add_layer(browser_layer);
#else
    TerminalLayer term_layer(std::make_unique<Terminal>(), 60);
    app->add_layer(term_layer);
#endif

    app->add_layer(logging::LogLayer {});
    app->add_layer(core::input::InputLayer {});

    app->scheduler.add_system(ecs::Stage::PreUpdate, handle_input);

    app->run();
    return 0;
}
