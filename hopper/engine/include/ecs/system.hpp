#pragma once

#include <functional>
#include <unordered_map>
#include <utility>

#include "ecs/world.hpp"

namespace ecs {

enum class SystemStage {
    Startup,
    PreUpdate,
    Update,
    FixedUpdate,
};

using System = std::function<void(World&)>;

class SystemScheduler {
public:
    void add_system(SystemStage stage, System&& system) {
        stages_[stage].push_back(std::forward<System>(system));
    }

    void run_stage(SystemStage stage, World& world) {
        for (auto system: stages_[stage]) {
            system(world);
        }
    }

private:
    std::unordered_map<SystemStage, std::vector<System>> stages_;
};

} // namespace ecs
