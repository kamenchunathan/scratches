#include "physics/systems.hpp"

#include <cmath>
#include <tuple>
#include <vector>

#include "ecs/entity.hpp"
#include "ecs/query.hpp"
#include "physics/components.hpp"
#include "physics/layer.hpp"
#include "physics/shape.hpp"
#include "time.hpp"
#include "types.hpp"
#include "util.hpp"

namespace physics {

void integrate(ecs::World& world) {
    auto config_opt = world.get_resource<PhysicsConfig>();
    auto time_opt   = world.get_resource<core::Time>();
    if (!time_opt)
        return;

    const float dt          = static_cast<float>(time_opt->get().delta_seconds);
    const Eigen::Vector2f g = config_opt ? config_opt->get().gravity
                                         : Eigen::Vector2f {0.f, -9.81f};

    auto query = ecs::Query<core::Transform, Rigidbody, Collider>(&world);
    for (auto [entity, transform, rb, collider]: query) {
        if (rb.body_type == RigidbodyType::Static)
            continue;

        // Integrate velocities
        if (rb.body_type == RigidbodyType::Dynamic) {
            const float inv_mass = rb.mass > 0.f ? 1.f / rb.mass : 0.f;

            rb.linear_velocity += g * dt;
            rb.linear_velocity += rb.accumulated_force() * inv_mass * dt;
        }

        // Integrate positions
        transform.set_translation_xy(transform.translation_xy() + (rb.linear_velocity * dt));
    }
}

struct CollisionPair {
    ecs::Entity a, b;
};

/// Converts a collider shape to AABB for broad phase.
/// Currently only AABB collider shapes are supported so it is trivial. Will be extended later
auto shape_to_aabb(core::Transform transform, Collider collider) -> Aabb {
    Eigen::Vector2f half_extents = std::visit(
        util::overload {[](AabbShape shape) { return shape.half_extents; }},
        collider.shape
    );

    return Aabb {
        .center       = transform.translation_xy(),
        .half_extents = half_extents,
    };
}

void detect_and_resolve(ecs::World& world) {
    auto query = ecs::Query<core::Transform, Collider>(&world);

    // Pre-calculate AABBs to avoid O(N^2) conversions and redundant calculations
    struct ColliderData {
        ecs::Entity entity;
        Collider collider;
        Aabb aabb;
    };

    std::vector<ColliderData> colliders;
    for (auto [entity, transform, col]: query) {
        colliders.push_back({entity, col, shape_to_aabb(transform, col)});
    }

    std::vector<CollisionPair> collision_pairs;

    for (std::uint32_t i = 0; i < colliders.size(); ++i) {
        const auto& a = colliders[i];

        for (std::uint32_t j = i + 1; j < colliders.size(); ++j) {
            const auto& b = colliders[j];

            // Filter collisions
            if ((a.collider.filter.mask_bits & b.collider.filter.category_bits) != 0
                && (b.collider.filter.mask_bits & a.collider.filter.category_bits) != 0)
            {
                // AABBs overlap only if they overlap on ALL axes
                if ((std::abs(a.aabb.center.x() - b.aabb.center.x())
                     < (a.aabb.half_extents.x() + b.aabb.half_extents.x()))
                    && (std::abs(a.aabb.center.y() - b.aabb.center.y())
                        < (a.aabb.half_extents.y() + b.aabb.half_extents.y())))
                {
                    collision_pairs.push_back(CollisionPair {a.entity, b.entity});
                }
            }
        }
    }
}

} // namespace physics
