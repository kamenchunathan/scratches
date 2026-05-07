#pragma once

#include <Eigen/Dense>

#include "application.hpp"

namespace physics {

struct PhysicsLayer {
    auto build(std::shared_ptr<core::Application>) -> void;
};

} // namespace physics
