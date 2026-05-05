#include <memory>
#include <print>

#include "application.hpp"
#include "input.hpp"
#include "term/layer.hpp"

auto startup(ecs::World&) {
    std::print("Engine: Reading input state from the engine. Press 'q' or 'escape' to quit.\r\n");
}

auto input_handling(ecs::World& world) -> void {
    std::print("Engine: Processing Input (PreUpdate)\r\n");

    if (auto input_state = world.get_resource<core::input::InputState>(); input_state.has_value()) {
        auto inp = input_state->get();
        if (inp.just_pressed(core::input::KeyCode::Q)
            || inp.just_pressed(core::input::KeyCode::Escape))
        {
            if (auto app_opt = world.get_resource<core::Application*>(); app_opt.has_value()) {
                app_opt->get()->set_should_exit(true);
            }
            return;
        }

        for (int i = 1; i < 100; ++i) {
            auto kc = static_cast<core::input::KeyCode>(i);
            if (inp.just_pressed(kc)) {
                std::print("Key pressed: {}\r\n", kc);
            }
        }

        for (int i = 0; i < 3; ++i) {
            auto mb = static_cast<core::input::MouseButton>(i);
            if (inp.just_pressed(mb)) {
                std::print(
                    "Mouse pressed: {} at ({}, {})\r\n",
                    mb,
                    inp.mouse_pos.second,
                    inp.mouse_pos.first
                );
            }
        }
    }
}

// ============================================================================
// Physics Layer
// ============================================================================

struct BroadPhaseSet {};
struct NarrowPhaseSet {};
struct IntegrateSet {};

void init_physics(ecs::World&) {
    std::print("Physics: Startup (init_physics)\r\n");
}

void integrate_velocities(ecs::World&) {
    std::print("Physics: FixedUpdate (integrate_velocities)\r\n");
}

void broad_phase_check(ecs::World&) {
    std::print("Physics: Update (broad_phase_check) - Should run BEFORE narrow phase\r\n");
}

void narrow_phase_resolve(ecs::World&) {
    std::print("Physics: Update (narrow_phase_resolve) - Should run AFTER broad phase\r\n");
}

struct PhysicsLayer {
    void build(std::shared_ptr<core::Application> app) {
        // Configure set ordering
        app->scheduler.configure_set<IntegrateSet>(ecs::Stage::FixedUpdate);
        app->scheduler.configure_set<BroadPhaseSet>(ecs::Stage::Update).before<NarrowPhaseSet>();
        app->scheduler.configure_set<NarrowPhaseSet>(ecs::Stage::Update);

        // Add systems to the schedule
        app->scheduler.add_system(ecs::Stage::Startup, init_physics);
        app->scheduler.add_system(ecs::Stage::FixedUpdate, integrate_velocities)
            .in_set<IntegrateSet>();
        app->scheduler.add_system(ecs::Stage::Update, broad_phase_check).in_set<BroadPhaseSet>();
        app->scheduler.add_system(ecs::Stage::Update, narrow_phase_resolve)
            .in_set<NarrowPhaseSet>();
    }
};

int main() {
    auto app = std::make_shared<core::Application>();
    auto term_layer = TerminalLayer(std::make_unique<Terminal>(), 30);

    app->world.insert_resource(app.get());
    app->add_layer(term_layer);
    app->add_layer(core::input::InputLayer {});
    app->add_layer(PhysicsLayer {});

    app->scheduler.add_system(ecs::Stage::Startup, startup);
    app->scheduler.add_system(ecs::Stage::PreUpdate, input_handling);

    app->run();

    return 0;
}
