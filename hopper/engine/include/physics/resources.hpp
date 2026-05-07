#pragma once

#include <Eigen/Dense>

#include "ecs/entity.hpp"

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

struct CollisionEvent {
    ecs::Entity entity_a;
    ecs::Entity entity_b;
};

struct Collisions {
    std::vector<CollisionEvent> events;
};

} // namespace physics
