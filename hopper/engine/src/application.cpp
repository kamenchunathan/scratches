#include "application.hpp"
#include "log.hpp"

#include "ecs/system.hpp"
#include "time.hpp"

namespace core {

void Application::set_runner(Runner runner) {
    runner_ = std::move(runner);
}

void Application::run() {
    HOPPER_INFO("Starting application");
    scheduler.run_stage(ecs::Stage::Startup, world);
    if (runner_) {
        runner_(shared_from_this());
    }
}

void Application::tick(double delta_time) {
    scheduler.run_stage(ecs::Stage::PreUpdate, world);

    if (!world.get_resource<core::FixedUpdateConfig>()) {
        world.insert_resource(core::FixedUpdateConfig {});
    }
    if (!world.get_resource<core::FixedUpdateAccumulator>()) {
        world.insert_resource(core::FixedUpdateAccumulator {});
    }

    auto& config      = world.get_resource<core::FixedUpdateConfig>()->get();
    auto& accumulator = world.get_resource<core::FixedUpdateAccumulator>()->get();

    if (auto opt_time = world.get_resource<core::Time>()) {
        auto& time         = opt_time->get();
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

    accumulator.accumulated_seconds += delta_time;

    while (accumulator.accumulated_seconds >= config.fixed_delta_seconds) {
        // Temporarily set delta_seconds to fixed_delta_seconds for FixedUpdate systems
        double original_delta = 0.0;
        if (auto opt_time = world.get_resource<core::Time>()) {
            auto& time         = opt_time->get();
            original_delta     = time.delta_seconds;
            time.delta_seconds = config.fixed_delta_seconds;
        }

        scheduler.run_stage(ecs::Stage::FixedUpdate, world);

        if (auto opt_time = world.get_resource<core::Time>()) {
            opt_time->get().delta_seconds = original_delta;
        }

        accumulator.accumulated_seconds -= config.fixed_delta_seconds;
    }

    scheduler.run_stage(ecs::Stage::Update, world);
    scheduler.run_stage(ecs::Stage::PostUpdate, world);
}

} // namespace core
