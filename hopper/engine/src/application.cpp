#include "application.hpp"

#include "ecs/system.hpp"
#include "log.hpp"
#include "profile.hpp"
#include "time.hpp"

namespace core {

void Application::set_runner(Runner runner) {
    runner_ = std::move(runner);
}

void Application::run() {
    HOPPER_INFO("engine", "application starting");
    scheduler.run_stage(ecs::Stage::Startup, world);
    if (runner_) {
        runner_(shared_from_this());
    }
    HOPPER_INFO("engine", "application exiting");

    // TODO: Create a way for applications to handle exit call backs and register this there
    //
    //
    logging::shutdown();
}

void Application::tick(double delta_time) {
    // HOPPER_FRAME_MARK();
    HOPPER_ZONE_NAMED("tick");

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
        double original_delta = 0.0;
        if (auto opt_time = world.get_resource<core::Time>()) {
            auto& time         = opt_time->get();
            original_delta     = time.delta_seconds;
            time.delta_seconds = config.fixed_delta_seconds;
        }

        {
            HOPPER_ZONE_NAMED("FixedUpdate");
            scheduler.run_stage(ecs::Stage::FixedUpdate, world);
        }

        if (auto opt_time = world.get_resource<core::Time>()) {
            opt_time->get().delta_seconds = original_delta;
        }

        accumulator.accumulated_seconds -= config.fixed_delta_seconds;
    }

    {
        HOPPER_ZONE_NAMED("PreUpdate");
        scheduler.run_stage(ecs::Stage::PreUpdate, world);
    }
    {
        HOPPER_ZONE_NAMED("Update");
        scheduler.run_stage(ecs::Stage::Update, world);
    }
    {
        HOPPER_ZONE_NAMED("PostUpdate");
        scheduler.run_stage(ecs::Stage::PostUpdate, world);
    }
}

} // namespace core
