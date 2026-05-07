#pragma once

#include <Eigen/Dense>

#include "application.hpp"

namespace physics {

/// An ECS resource to control global configuration
struct PhysicsConfig {
    Eigen::Vector2f gravity = {0.f, -9.8f};

    /// Number of substeps the solver
    std::uint32_t substeps = 4;

    /// Positional correction (projection method)
    /// Overlap below slop is ignored to prevent jitter at rest contacts.
    float position_correction_slop = 0.005f;
};

struct PhysicsLayer {
    auto build(std::shared_ptr<core::Application>) -> void;
};

} // namespace physics
