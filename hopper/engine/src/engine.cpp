#include "engine.hpp"
#include <utility>

namespace core {

Application::~Application() = default;

void Application::set_runner(Runner runner) {
    runner_ = std::move(runner);
}

void Application::run() {
    if (runner_) {
        runner_(*this);
    }
}

} // namespace core
