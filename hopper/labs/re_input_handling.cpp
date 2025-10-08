#include <print>
#include <utility>

#include "application.hpp"
#include "input.hpp"
#include "term/layer.hpp"

int main() {
    core::Application app;
    TerminalLayer term_layer {.terminal = std::make_unique<Terminal>()};
    app.add_layer(term_layer);
    app.add_layer(core::input::InputLayer {});

    app.scheduler.add_system(ecs::SystemStage::Startup, [](ecs::World&) {
        std::print("Reading input state from the engine. Press 'q' or 'escape' to quit.\r\n");
    });

    app.scheduler.add_system(ecs::SystemStage::Update, [&app](ecs::World& world) {
        if (auto* input_state = world.get_resource<core::input::InputState>()) {
            if (input_state->just_pressed(core::input::KeyCode::Q)
                || input_state->just_pressed(core::input::KeyCode::Escape))
            {
                app.set_should_exit(true);
                return;
            }

            // Iterate over members of keycode enum
            // NOTE: postprocessing is disabled in raw mode therefore we have to manually add
            // carriage return and linefeed
            for (int i = 1; i < 100; ++i) {
                auto kc = static_cast<core::input::KeyCode>(i);
                if (input_state->just_pressed(kc)) {
                    std::print("Key pressed: {}\r\n", kc);
                }
            }

            for (int i = 0; i < 3; ++i) {
                auto mb = static_cast<core::input::MouseButton>(i);
                if (input_state->just_pressed(mb)) {
                    std::print(
                        "Mouse pressed: {} at ({}, {})\r\n",
                        mb,
                        input_state->mouse_pos.second,
                        input_state->mouse_pos.first
                    );
                }
            }
        }
    });

    app.run();

    return 0;
}
