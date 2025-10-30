#pragma once

#include <functional>
#include <memory>

#include "ecs/system.hpp"
#include "ecs/world.hpp"

namespace core {

class Application;

template<typename T>
concept Layer = requires(T t, std::shared_ptr<Application> app) {
    { t.build(app) } -> std::same_as<void>;
};

class Application: public std::enable_shared_from_this<Application> {
public:
    ecs::World world;
    ecs::SystemScheduler scheduler;

    Application() = default;
    ~Application() = default;

    using Runner = std::function<void(std::shared_ptr<Application>)>;

    template<Layer L>
    void add_layer(L&& layer);

    void set_runner(Runner);

    void run();

    /* delta_time: Milliseconds */
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
    layer.build(shared_from_this());
}

} // namespace core
