#include "physics/shape.hpp"

#include "physics/components.hpp"
#include "util.hpp"

namespace physics {

/// Converts a collider shape to AABB for broad phase.
/// Currently only AABB collider shapes are supported so it is trivial. Will be extended later
[[nodiscard]] auto Aabb::from_shape(core::Transform transform, Collider collider) -> Aabb {
    Eigen::Vector2f half_extents = std::visit(
        util::overload {[](AabbShape shape) { return shape.half_extents; }},
        collider.shape
    );

    return Aabb {
        .center       = transform.translation_xy(),
        .half_extents = half_extents,
    };
}

} // namespace physics
