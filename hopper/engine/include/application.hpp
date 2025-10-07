#pragma once

#include <functional>

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
    Application() = default;
    ~Application() = default;

    ecs::World world;
    using Runner = std::function<void(Application&)>;

    template<Layer L>
    void add_layer(L layer);

    void set_runner(Runner);
    void run();

private:
    Runner runner_;
};

template<Layer L>
void Application::add_layer(L layer) {
    layer.build(*this);
}

} // namespace core
