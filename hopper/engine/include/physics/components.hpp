#pragma once

#include <Eigen/Dense>

#include "physics/shape.hpp"

namespace physics {

enum class RigidbodyType : std::uint8_t {
    /// Does not have any mass and is immovable
    Static,

    ///  Velocities and position managed by physics system based on mass
    /// Accumulation of forces -> integration to get velocity and position
    /// Controlled by application of forces
    Dynamic,

    /// Velocities are programmer controlled.  Does not respond to world forces e.g. gravity
    /// Interacts with dynamic objects as though it were immovable
    /// Used for custom character controllers etc.
    Kinematic
};

/// Controls pairs tested by narrow phase
struct CollisionFilter {
    std::uint32_t category_bits = 0xFFFF'FFFF;
    std::uint32_t mask_bits     = 0xFFFF'FFFF;
};

struct Rigidbody {
    Eigen::Vector2f linear_velocity = Eigen::Vector2f::Zero();
    float mass                      = 0.f;
    float restitution               = 0.3f;
    RigidbodyType body_type         = RigidbodyType::Dynamic;

    [[nodiscard]] auto accumulated_force() const -> Eigen::Vector2f const;
    auto apply_impulse(Eigen::Vector2f) -> void;
    auto apply_force(Eigen::Vector2f) -> void;

private:
    /// Accumulates forces applied between ticks. Editted by a public method to apply forces
    /// Cleared every fixed update
    Eigen::Vector2f accumulated_force_ = Eigen::Vector2f::Zero();
};

struct Collider {
    Shape shape;
    CollisionFilter filter;
    /// fire collision events but does not produce  impulses
    bool is_trigger;
};

} // namespace physics
