#include "physics/components.hpp"

namespace physics {

auto Rigidbody::apply_impulse(Eigen::Vector2f impulse) -> void {
    linear_velocity += impulse / mass;
}

auto Rigidbody::apply_force(Eigen::Vector2f f) -> void {
    accumulated_force_ += f;
}

} // namespace physics
