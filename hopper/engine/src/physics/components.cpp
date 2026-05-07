#include "physics/components.hpp"

namespace physics {

auto Rigidbody::apply_impulse(Eigen::Vector2f impulse) -> void {
    linear_velocity += impulse / mass;
}

auto Rigidbody::apply_force(Eigen::Vector2f f) -> void {
    accumulated_force_ += f;
}

[[nodiscard]] auto Rigidbody::accumulated_force() const -> Eigen::Vector2f {
    return accumulated_force_;
}

auto Rigidbody::inv_mass() const -> float {
    if (body_type == RigidbodyType::Dynamic) {
        return mass > 0.f ? 1.f / mass : 0.f;
    }
    return 0.f;
}

} // namespace physics
