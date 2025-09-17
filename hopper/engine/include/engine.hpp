#pragma once

#include "ecs/world.hpp"
#include <memory>

namespace core {
class Engine {
public:
    Engine();
    ~Engine();

    void initialize();
    void shutdown();
    void run();

    ecs::World& get_world() {
        return *world_;
    }
    const ecs::World& get_world() const {
        return *world_;
    }

private:
    std::unique_ptr<ecs::World> world_;
};
} // namespace core
