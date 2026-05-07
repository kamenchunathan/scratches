#pragma once

#include <variant>

#include <Eigen/Dense>

#include "types.hpp"

namespace physics {

struct Collider;

struct AabbShape {
    Eigen::Vector2f half_extents;
};

/// Collider shapes
using Shape = std::variant<AabbShape>;

// World-space axis-aligned bounding box
// Used as both a query primitive and the runtime representation of AabbShape.
struct Aabb {
    Eigen::Vector2f center;
    Eigen::Vector2f half_extents;

    [[nodiscard]] Eigen::Vector2f min() const noexcept {
        return center - half_extents;
    }
    [[nodiscard]] Eigen::Vector2f max() const noexcept {
        return center + half_extents;
    }

    [[nodiscard]] static auto from_shape(core::Transform transform, Collider collider) -> Aabb;
};

} // namespace physics
