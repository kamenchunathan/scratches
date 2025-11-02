#include "application.hpp"
#include "ecs/system.hpp"
#include "time.hpp"

namespace core {

void Application::set_runner(Runner runner) {
    runner_ = std::move(runner);
}

void Application::run() {
    scheduler.run_stage(ecs::SystemStage::Startup, world);
    if (runner_) {
        runner_(shared_from_this());
    }
}

void Application::tick(double delta_time) {
    if (auto opt_time = world.get_resource<core::Time>()) {
        auto& time = opt_time->get();
        time.delta_seconds = delta_time;
        time.total_seconds += delta_time;
    } else {
        world.insert_resource(
            core::Time {
                .delta_seconds = delta_time,
                .total_seconds = delta_time,
            }
        );
    }

    scheduler.run_stage(ecs::SystemStage::Update, world);
}

} // namespace core
