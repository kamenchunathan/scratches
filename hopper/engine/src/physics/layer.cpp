#include "physics/layer.hpp"
#include "ecs/system.hpp"
#include "physics/resources.hpp"
#include "physics/systems.hpp"

namespace physics {

/// Systemsets for ordering of physics phases
/// IntegrateSet -> CollisionResolveSet -> CleanupSet
struct IntegrateSet {};
struct CollisionResolveSet {};
struct CleanupSet {};

auto PhysicsLayer::build(std::shared_ptr<core::Application> app) -> void {
    if (!app->world.has_resource<PhysicsConfig>()) {
        app->world.insert_resource(PhysicsConfig {});
    }
    if (!app->world.has_resource<Collisions>()) {
        app->world.insert_resource(Collisions {});
    }

    app->scheduler.configure_set<IntegrateSet>(ecs::Stage::FixedUpdate);
    app->scheduler.configure_set<CollisionResolveSet>(ecs::Stage::FixedUpdate)
        .after<IntegrateSet>();
    app->scheduler.configure_set<CleanupSet>(ecs::Stage::FixedUpdate).after<CollisionResolveSet>();

    app->scheduler.add_system(ecs::Stage::FixedUpdate, integrate).in_set<IntegrateSet>();
    app->scheduler.add_system(ecs::Stage::FixedUpdate, detect_and_resolve)
        .in_set<CollisionResolveSet>();
    app->scheduler.add_system(ecs::Stage::FixedUpdate, clear_forces).in_set<CleanupSet>();
}

} // namespace physics
