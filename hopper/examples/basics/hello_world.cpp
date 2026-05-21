#include <memory>

#include "application.hpp"
#include "ecs/system.hpp"
#include "ecs/world.hpp"
#include "input.hpp"
#include "log.hpp"
#include "log/layer.hpp"

// Include platform-specific rendering layers
#if PLATFORM_WASM
    #include "web/layer.hpp"
#else
    #include "term/layer.hpp"
#endif

// A startup system runs exactly once when the application starts
void startup_system(ecs::World& world) {
    HOPPER_INFO("app", "Application starting up!");
}

// A pre-update system runs every frame before the main logic
void handle_input(ecs::World& world) {
    if (auto inp_state = world.get_resource<core::input::InputState>(); inp_state.has_value()) {
        auto inp = inp_state->get();
        if (inp.just_pressed(core::input::KeyCode::Q)) {
            if (auto app = world.get_resource<core::Application*>(); app.has_value()) {
                HOPPER_INFO("app", "Quit requested, exiting...");
                app->get()->set_should_exit(true);
            }
        }
    }
}

int main() {
    // 1. Create the core application
    // Note: We use a static pointer so the application instance persists past main().
    // This is required for WebAssembly builds where the run function exits early
    // and execution continues in an asynchronous browser loop.
    static auto app = std::make_shared<core::Application>();
    app->world.insert_resource<core::Application*>(app.get());

    // 2. Add essential layers (like logging and inputs)
    app->add_layer(logging::LogLayer {});
    app->add_layer(core::input::InputLayer {});

    // 3. Add platform-specific rendering layer (Terminal vs Web)
#if PLATFORM_WASM
    web::BrowserLayer browser_layer {};
    app->add_layer(browser_layer);
#else
    TerminalLayer term_layer(std::make_unique<Terminal>(), 60);
    app->add_layer(term_layer);
#endif

    // 4. Schedule systems
    // ecs::Stage::Startup executes once at application launch
    app->scheduler.add_system(ecs::Stage::Startup, startup_system);
    // ecs::Stage::PreUpdate executes every frame to process input before game logic
    app->scheduler.add_system(ecs::Stage::PreUpdate, handle_input);

    // 5. Run the main game loop
    app->run();
    return 0;
}
