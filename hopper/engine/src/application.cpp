#include <print>

#include "application.hpp"
#include "ecs/system.hpp"

namespace core {

void Application::set_runner(Runner runner) {
    runner_ = std::move(runner);
}

void Application::run() {
    scheduler.run_stage(ecs::SystemStage::Startup, world);
    if (runner_) {
        runner_(*this);
    }
}

void Application::tick(double) {
    // std::println("{}", should_exit());
    scheduler.run_stage(ecs::SystemStage::Update, world);
}

} // namespace core
