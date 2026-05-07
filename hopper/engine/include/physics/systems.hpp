#pragma once

#include "ecs/world.hpp"

namespace physics::systems {

/// Integrate forces, gravity, damping → update velocity → update Transform position/yaw.
/// Runs first in FixedUpdate so positions are correct before detection.
void integrate(ecs::World& world);

/// Broad phase (O(n²) AABB sweep) + narrow phase (exact manifold) +
/// impulse resolution + positional correction, all in one pass.
/// Sensors are detected but never impulse-resolved.
/// Writes results into per-frame contact sets stored on the ContactCache resource.
void detect_and_resolve(ecs::World& world);

} // namespace physics::systems
