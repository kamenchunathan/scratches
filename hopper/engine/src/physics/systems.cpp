#include "physics/systems.hpp"

#include <cmath>
#include <vector>

#include "ecs/query.hpp"
#include "physics/components.hpp"
#include "physics/layer.hpp"
#include "physics/shape.hpp"
#include "time.hpp"

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
    std::size_t a, b;
};

struct ContactManifold {
    /// unit vector from B toward A
    Eigen::Vector2f normal;
    /// penetration depth (positive when overlapping)
    float depth;
    /// approximate world-space contact point
    // Not necessary at this point
    // Eigen::Vector2f point;
};

auto compute_contact_manifold(
    const Aabb& a,
    const Aabb& b,
    std::float_t overlap_x,
    std::float_t overlap_y
) -> ContactManifold {
    ContactManifold manifold;

    if (overlap_x < overlap_y) {
        manifold.depth = overlap_x;
        // Normal from B to A
        if (a.center.x() < b.center.x()) {
            manifold.normal = {-1, 0};
        } else {
            manifold.normal = {1, 0};
        }
    } else {
        manifold.depth = overlap_y;
        // Normal from B to A
        if (a.center.y() < b.center.y()) {
            manifold.normal = {0, -1};
        } else {
            manifold.normal = {0, 1};
        }
    }

    return manifold;
}

auto resolve_pair(
    Rigidbody& ra,
    Rigidbody& rb,
    core::Transform& ta,
    core::Transform& tb,
    const ContactManifold& manifold,
    float slop
) -> void {
    const float inv_ma         = ra.inv_mass();
    const float inv_mb         = rb.inv_mass();
    const float total_inv_mass = inv_ma + inv_mb;

    // Both immovable, return
    if (total_inv_mass == 0.f)
        return;

    const Eigen::Vector2f& n = manifold.normal;

    // 1. Velocity Resolution (Impulse)
    const Eigen::Vector2f rel_vel = ra.linear_velocity - rb.linear_velocity;
    const float vel_along_normal  = rel_vel.dot(n);

    // Do not resolve if velocities are separating
    if (vel_along_normal > 0.f)
        return;

    const float e = std::max(ra.restitution, rb.restitution);

    float j = -(1.f + e) * vel_along_normal;
    j /= total_inv_mass;

    const Eigen::Vector2f impulse = j * n;
    ra.linear_velocity += inv_ma * impulse;
    rb.linear_velocity -= inv_mb * impulse;

    // 2. Position Correction (Linear Projection)
    const float percent = 0.2f; // penetration percentage to correct
    const Eigen::Vector2f correction
        = (std::max(manifold.depth - slop, 0.0f) / total_inv_mass) * percent * n;

    ta.set_translation_xy(ta.translation_xy() + inv_ma * correction);
    tb.set_translation_xy(tb.translation_xy() - inv_mb * correction);
}

void detect_and_resolve(ecs::World& world) {
    auto config = world.get_resource<PhysicsConfig>()->get();

    auto query = ecs::Query<core::Transform, Rigidbody, Collider>(&world);

    // Pre-calculate AABBs to avoid O(N^2) conversions and redundant calculations
    struct ColliderData {
        Aabb aabb;
        core::Transform& transform;
        Collider& collider;
        Rigidbody& body;
    };

    std::vector<ColliderData> colliders;
    std::vector<CollisionPair> collision_pairs;

    colliders.clear();
    collision_pairs.clear();

    for (auto [entity, transform, rb, col]: query) {
        colliders.push_back({
            .aabb      = Aabb::from_shape(transform, col),
            .transform = transform,
            .collider  = col,
            .body      = rb,
        });
    }

    for (std::size_t i = 0; i < colliders.size(); ++i) {
        const auto& a = colliders[i];

        for (std::size_t j = i + 1; j < colliders.size(); ++j) {
            const auto& b = colliders[j];

            // Filter collisions
            if ((a.collider.filter.mask_bits & b.collider.filter.category_bits) != 0
                && (b.collider.filter.mask_bits & a.collider.filter.category_bits) != 0)
            {
                if (a.aabb.intersects(b.aabb)) {
                    collision_pairs.push_back({.a = i, .b = j});
                }
            }
        }
    }

    for (const auto& pair: collision_pairs) {
        auto& a = colliders[pair.a];
        auto& b = colliders[pair.b];

        // Re-calculate AABBs to use latest transform state (handles multiple collisions/frame)
        Aabb a_aabb = Aabb::from_shape(a.transform, a.collider);
        Aabb b_aabb = Aabb::from_shape(b.transform, b.collider);

        auto overlap_x = (a_aabb.half_extents.x() + b_aabb.half_extents.x())
            - std::abs(a_aabb.center.x() - b_aabb.center.x());
        auto overlap_y = (a_aabb.half_extents.y() + b_aabb.half_extents.y())
            - std::abs(a_aabb.center.y() - b_aabb.center.y());

        if (overlap_x <= 0 || overlap_y <= 0) {
            continue;
        }

        // Early break for triggers
        if (a.collider.is_trigger || b.collider.is_trigger) {
            // TODO: Fire events
            continue;
        }

        ContactManifold manifold = compute_contact_manifold(a_aabb, b_aabb, overlap_x, overlap_y);

        resolve_pair(
            a.body,
            b.body,
            a.transform,
            b.transform,
            manifold,
            config.position_correction_slop
        );
    }
}

auto clear_forces(ecs::World& world) -> void {
    for (auto [_, rb]: ecs::Query<Rigidbody>(&world)) {
        rb.clear_accumulated_forces();
    }
}

} // namespace physics
