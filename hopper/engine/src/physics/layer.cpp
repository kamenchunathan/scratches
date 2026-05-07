#include "physics/layer.hpp"
#include "ecs/system.hpp"
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

    auto sched = app->scheduler;

    sched.configure_set<IntegrateSet>(ecs::Stage::FixedUpdate);
    sched.configure_set<CollisionResolveSet>(ecs::Stage::FixedUpdate).after<IntegrateSet>();
    sched.configure_set<CleanupSet>(ecs::Stage::FixedUpdate).after<CollisionResolveSet>();

    sched.add_system(ecs::Stage::FixedUpdate, systems::integrate).in_set<IntegrateSet>();
    sched.add_system(ecs::Stage::FixedUpdate, systems::detect_and_resolve)
        .in_set<CollisionResolveSet>();
    sched.add_system(ecs::Stage::FixedUpdate, systems::emit_events).in_set<CollisionResolveSet>();
    sched.add_system(ecs::Stage::FixedUpdate, systems::clear_forces).in_set<CleanupSet>();
}

} // namespace physics
