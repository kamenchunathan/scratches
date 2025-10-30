#include <cstddef>
#include <optional>
#include <utility>
#include <variant>

#include "input.hpp"

namespace core::input {

void InputLayer::build(std::shared_ptr<core::Application> app) {
    app->world.insert_resource(InputState {});
}

template<class... Ts>
struct overload: Ts... {
    using Ts::operator()...;
};

template<class... Ts>
overload(Ts...) -> overload<Ts...>;

bool InputState::just_pressed(KeyCode key) {
    return keys_current[static_cast<std::size_t>(key)]
        && !keys_previous[static_cast<std::size_t>(key)];
}

bool InputState::just_pressed(MouseButton btn) {
    return mouse_button_current[static_cast<std::size_t>(btn)]
        && !mouse_button_previous[static_cast<std::size_t>(btn)];
}

bool InputState::just_released(KeyCode key) {
    return !keys_current[static_cast<std::size_t>(key)]
        && keys_previous[static_cast<std::size_t>(key)];
}

bool InputState::just_released(MouseButton btn) {
    return !mouse_button_current[static_cast<std::size_t>(btn)]
        && mouse_button_previous[static_cast<std::size_t>(btn)];
}

bool InputState::is_button_down(KeyCode key) {
    return keys_current[static_cast<std::size_t>(key)];
}

bool InputState::is_button_down(MouseButton btn) {
    return mouse_button_current[static_cast<std::size_t>(btn)];
}

void InputState::process_events(std::vector<Event> events) {
    std::swap(keys_current, keys_previous);
    keys_current.fill(false);

    // Mouse button state remains the same unless explicitly changed by release or button down
    // events
    mouse_button_previous = mouse_button_current;

    for (auto event: events) {
        std::visit(
            overload {
                [this](core::input::KeyEvent key) {
                    keys_current[static_cast<std::size_t>(key.code)] = true;
                },
                [this](core::input::MouseEvent ev) {
                    switch (ev.action) {
                        case MouseEvent::Action::Press:
                            // Mouse button is guaranteed to be present on press
                            mouse_button_current[static_cast<std::size_t>(ev.button.value())] =
                                true;
                            mouse_pos = std::make_pair(ev.row, ev.col);
                            break;

                        case MouseEvent::Action::Move:
                            mouse_pos = std::make_pair(ev.row, ev.col);
                            break;

                        case MouseEvent::Action::Release:
                            mouse_button_current[static_cast<std::size_t>(ev.button.value())] =
                                false;
                            mouse_pos = std::make_pair(ev.row, ev.col);
                            break;
                    }
                },
                [](core::input::ResizeEvent) {
                    // TODO: Handle this
                }
            },
            event
        );
    }
}

} // namespace core::input
