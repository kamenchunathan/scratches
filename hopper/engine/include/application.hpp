#pragma once

#include <functional>

#include "ecs/system.hpp"
#include "ecs/world.hpp"

namespace core {

class Application;

template<typename T>
concept Layer = requires(T t, core::Application& app) {
    {
        t.build(app)
    } -> std::same_as<void>;
};

class Application {
public:
    ecs::World world;
    ecs::SystemScheduler scheduler;

    Application() = default;
    ~Application() = default;

    using Runner = std::function<void(Application&)>;

    // BUG: If a layer is passed as an lvalue it happens to be layer that sets the runner,
    // the value is dropped but any references to the layer captured by the runner remain
    // which may lead to segfaults.
    // Brainstorm approaches to this problem
    template<Layer L>
    void add_layer(L&& layer);

    void set_runner(Runner);

    void run();

    void tick(double delta_time);

    bool should_exit() {
        return should_exit_;
    }

    void set_should_exit(bool should_exit) {
        should_exit_ = should_exit;
    }

private:
    Runner runner_;
    // TODO: Replace with event
    bool should_exit_ = false;
};

template<Layer L>
void Application::add_layer(L&& layer) {
    layer.build(*this);
}

} // namespace core
