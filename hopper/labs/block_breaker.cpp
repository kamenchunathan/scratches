#include <Eigen/Dense>
#include <algorithm>
#include <format>
#include <memory>
#include <vector>

#include "application.hpp"
#include "color.hpp"
#include "ecs/query.hpp"
#include "ecs/system.hpp"
#include "ecs/world.hpp"
#include "input.hpp"
#include "physics/components.hpp"
#include "physics/layer.hpp"
#include "physics/resources.hpp"
#include "physics/shape.hpp"
#include "renderer.hpp"
#include "renderer/command.hpp"
#include "renderer/graphs/halfblock.hpp"
#include "renderer/resource.hpp"
#include "renderer/shader.hpp"
#include "renderer/types.hpp"
#include "transform.hpp"

#if PLATFORM_WASM
    #include "web/layer.hpp"
    #include "web/presenter.hpp"
#else
    #include "term/layer.hpp"
#endif

// --- Game State & Components ---

enum class GameState { Playing, GameOver };

struct GameStatus {
    GameState state = GameState::Playing;
};

struct PaddleTag {};
struct BallTag {};
struct BlockTag {};

struct RenderColor {
    core::ColorRGBA32F color;
};
struct Score {
    int value = 0;
};
struct Lives {
    int value = 5;
};

// --- Shaders and Render Commands ---

struct Vertex {
    Eigen::Vector2f position;
};
struct VOut {
    Eigen::Vector4f position;
    core::ColorRGBA32F color;
};

class QuadShader:
    public renderer::
        ShaderPrev<Vertex, VOut, core::ColorRGBA32F, Eigen::Matrix4f, core::ColorRGBA32F> {
public:
    VOut vertex(
        const Vertex& v,
        const Eigen::Matrix4f& model,
        const core::ColorRGBA32F& color
    ) override {
        Eigen::Vector4f world_pos
            = model * Eigen::Vector4f(v.position.x(), v.position.y(), 0.0f, 1.0f);
        return {world_pos, color};
    }
    core::ColorRGBA32F fragment(
        const VOut& v,
        const Eigen::Matrix4f& model,
        const core::ColorRGBA32F& color
    ) override {
        (void)v;
        (void)model;
        return color;
    }
};

using QuadPipeline
    = renderer::Pipeline<Vertex, VOut, core::ColorRGBA32F, Eigen::Matrix4f, core::ColorRGBA32F>;

struct GameResources {
    renderer::BufferHandle<Vertex> quad_buffer;
    renderer::TextureHandle<core::ColorRGBA32F> color_texture;
    renderer::TextureHandle<renderer::CharacterPixel> char_texture;
    renderer::PipelineHandle<QuadPipeline> pipeline;
    std::uint32_t screen_width;
    std::uint32_t screen_height;
};

class QuadRenderCommand: public renderer::RenderCommand {
public:
    QuadRenderCommand(
        renderer::PipelineHandle<QuadPipeline> pipeline,
        renderer::FrameBuffer<renderer::Attachment<core::ColorRGBA32F>> target,
        renderer::BufferHandle<Vertex> vertex_buffer,
        Eigen::Matrix4f model,
        core::ColorRGBA32F color
    ):
        pipeline_(pipeline),
        target_(target),
        vertex_buffer_(vertex_buffer),
        model_(model),
        color_(color) {}
    const std::string target_pass() const override {
        return renderer::halfblock::COLOR_PASS_FG;
    }
    void execute(renderer::RenderPassEncoder& encoder) override {
        encoder.draw(pipeline_, target_, vertex_buffer_, std::make_tuple(model_, color_), 6);
    }

private:
    renderer::PipelineHandle<QuadPipeline> pipeline_;
    renderer::FrameBuffer<renderer::Attachment<core::ColorRGBA32F>> target_;
    renderer::BufferHandle<Vertex> vertex_buffer_;
    Eigen::Matrix4f model_;
    core::ColorRGBA32F color_;
};

class TextRenderCommand: public renderer::RenderCommand {
public:
    TextRenderCommand(
        renderer::ResourceRegistry& registry,
        renderer::TextureHandle<renderer::CharacterPixel> target_texture,
        std::string text,
        std::uint32_t x,
        std::uint32_t y,
        core::ColorRGB8 fg = core::ColorRGB8::WHITE,
        core::ColorRGB8 bg = core::ColorRGB8::BLACK
    ):
        registry_(registry),
        target_texture_(target_texture),
        text_(text),
        x_(x),
        y_(y),
        fg_(fg),
        bg_(bg) {}
    const std::string target_pass() const override {
        return "ui_pass";
    }
    void execute(renderer::RenderPassEncoder&) override {
        auto tex_res = registry_.get_texture_mut(target_texture_);
        if (!tex_res)
            return;
        auto* tex = tex_res.value();
        for (std::size_t i = 0; i < text_.length(); ++i) {
            if (x_ + i >= tex->width())
                break;
            tex->data_mut()[y_ * tex->width() + x_ + i]
                = {.codepoint = static_cast<char32_t>(text_[i]), .fg_color = fg_, .bg_color = bg_};
        }
    }

private:
    renderer::ResourceRegistry& registry_;
    renderer::TextureHandle<renderer::CharacterPixel> target_texture_;
    std::string text_;
    std::uint32_t x_, y_;
    core::ColorRGB8 fg_, bg_;
};

// --- Level Generation ---

void spawn_blocks(ecs::World& world) {
    // Clear existing blocks
    std::vector<ecs::Entity> to_delete;
    for (auto [e, t]: ecs::Query<core::Transform>(&world).with<BlockTag>()) {
        to_delete.push_back(e);
    }
    for (auto e: to_delete)
        world.destroy(e);

    // Livelier neon palette for the blocks
    core::ColorRGBA32F row_colors[4] = {
        core::ColorRGBA32F::rgba(0.95f, 0.2f, 0.4f, 1.0f), // Vibrant Pink/Red
        core::ColorRGBA32F::rgba(0.95f, 0.5f, 0.1f, 1.0f), // Vibrant Orange
        core::ColorRGBA32F::rgba(0.6f, 0.85f, 0.1f, 1.0f), // Vibrant Lime
        core::ColorRGBA32F::rgba(0.1f, 0.75f, 0.95f, 1.0f) // Vibrant Cyan
    };

    // Adjusted play area size so blocks comfortably fit inside the new visible walls
    for (int y = 0; y < 4; ++y) {
        for (int x = -6; x <= 6; ++x) {
            world.spawn(
                core::Transform::from_pos_rot_scale(
                    {x * 11.5f, 20.0f + y * 7.0f, 0.0f},
                    Eigen::Quaternionf::Identity(),
                    {5.5f, 2.5f, 1.0f}
                ),
                physics::Rigidbody {
                    .mass        = 0.0f,
                    .restitution = 1.0f,
                    .body_type   = physics::RigidbodyType::Static
                },
                physics::Collider {
                    .shape      = physics::AabbShape {{1.0f, 1.0f}},
                    .filter     = {},
                    .is_trigger = false
                },
                RenderColor {row_colors[y]},
                BlockTag {}
            );
        }
    }
}

void spawn_ball(ecs::World& world) {
    world.spawn(
        core::Transform::from_pos_rot_scale(
            {0.0f, 0.0f, 0.0f},
            Eigen::Quaternionf::Identity(),
            {2.0f, 2.0f, 1.0f}
        ),
        physics::Rigidbody {
            .linear_velocity = {35.0f, 40.0f},
            .mass            = 1.0f,
            .restitution     = 1.0f,
            .body_type       = physics::RigidbodyType::Dynamic
        },
        physics::Collider {
            .shape      = physics::AabbShape {{1.0f, 1.0f}},
            .filter     = {},
            .is_trigger = false
        },
        RenderColor {core::ColorRGBA32F::WHITE},
        BallTag {}
    );
}

// --- Systems ---

void paddle_system(ecs::World& world) {
    if (auto status = world.get_resource<GameStatus>()) {
        if (status->get().state == GameState::GameOver)
            return; // Lock input
    }

    auto input_state_res = world.get_resource<core::input::InputState>();
    if (!input_state_res)
        return;
    auto& input = input_state_res->get();

    float move = 0.0f;
    if (input.is_button_down(core::input::KeyCode::A)
        || input.is_button_down(core::input::KeyCode::Left))
        move -= 1.0f;
    if (input.is_button_down(core::input::KeyCode::D)
        || input.is_button_down(core::input::KeyCode::Right))
        move += 1.0f;

    for (auto [entity, transform, rb]:
         ecs::Query<core::Transform, physics::Rigidbody>(&world).with<PaddleTag>())
    {
        rb.linear_velocity.x() = move * 80.0f;

        // Shrink the clamping zone to account for the newly visible walls (walls are at +/-76.0f inner boundary)
        if (transform.translation.x() < -61.0f && rb.linear_velocity.x() < 0)
            rb.linear_velocity.x() = 0;
        if (transform.translation.x() > 61.0f && rb.linear_velocity.x() > 0)
            rb.linear_velocity.x() = 0;
    }
}

void ball_system(ecs::World& world) {
    auto status_opt = world.get_resource<GameStatus>();
    if (!status_opt)
        return;
    auto& status = status_opt->get();

    auto input_state = world.get_resource<core::input::InputState>();
    if (!input_state)
        return;
    auto& input = input_state->get();

    // App Quit
    if (input.just_pressed(core::input::KeyCode::Escape)
        || input.just_pressed(core::input::KeyCode::Q))
    {
        if (auto app = world.get_resource<core::Application*>())
            app->get()->set_should_exit(true);
    }

    // Handle Game Over Restarts
    if (status.state == GameState::GameOver) {
        if (input.just_pressed(core::input::KeyCode::Space)) {
            status.state = GameState::Playing;
            if (auto lives = world.get_resource<Lives>())
                lives->get().value = 5;
            if (auto score = world.get_resource<Score>())
                score->get().value = 0;

            spawn_blocks(world);
            spawn_ball(world); // Respawn ball dynamically

            for (auto [e, t]: ecs::Query<core::Transform>(&world).with<PaddleTag>()) {
                t.translation = {0.0f, -40.0f, 0.0f};
            }
        }
        return;
    }

    std::vector<ecs::Entity> balls_to_destroy;

    // Ball bounds checks & life tracking
    for (auto [entity, transform, rb]:
         ecs::Query<core::Transform, physics::Rigidbody>(&world).with<BallTag>())
    {
        if (transform.translation.y() < -55.0f) {
            if (auto lives_opt = world.get_resource<Lives>()) {
                lives_opt->get().value -= 1;
                if (lives_opt->get().value <= 0) {
                    status.state = GameState::GameOver;
                    balls_to_destroy.push_back(entity); // Despawn ball on zero lives
                } else {
                    transform.translation = {0.0f, 0.0f, 0.0f};
                    rb.linear_velocity    = {35.0f, 40.0f};
                }
            }
        } else {
            float speed = rb.linear_velocity.norm();
            if (speed > 0.1f) {
                if (speed < 40.0f)
                    rb.linear_velocity = rb.linear_velocity.normalized() * 40.0f;
                else if (speed > 80.0f)
                    rb.linear_velocity = rb.linear_velocity.normalized() * 80.0f;
            }
        }
    }

    // Process despawns
    for (auto e: balls_to_destroy) {
        world.destroy(e);
    }
}

void block_breaking_system(ecs::World& world) {
    auto status = world.get_resource<GameStatus>();
    if (status && status->get().state == GameState::GameOver)
        return;

    auto collisions_opt = world.get_resource<physics::Collisions>();
    if (!collisions_opt)
        return;

    std::vector<ecs::Entity> blocks_to_destroy;

    for (const auto& ev: collisions_opt->get().events) {
        bool a_is_ball  = world.has_component<BallTag>(ev.entity_a);
        bool b_is_block = world.has_component<BlockTag>(ev.entity_b);

        bool b_is_ball  = world.has_component<BallTag>(ev.entity_b);
        bool a_is_block = world.has_component<BlockTag>(ev.entity_a);

        if (a_is_ball && b_is_block)
            blocks_to_destroy.push_back(ev.entity_b);
        if (b_is_ball && a_is_block)
            blocks_to_destroy.push_back(ev.entity_a);
    }

    if (!blocks_to_destroy.empty()) {
        std::sort(blocks_to_destroy.begin(), blocks_to_destroy.end());
        blocks_to_destroy.erase(
            std::unique(blocks_to_destroy.begin(), blocks_to_destroy.end()),
            blocks_to_destroy.end()
        );

        if (auto score_opt = world.get_resource<Score>()) {
            score_opt->get().value += static_cast<int>(blocks_to_destroy.size()) * 100;
        }
        for (auto e: blocks_to_destroy) {
            world.destroy(e);
        }
    }
}

void render_system(ecs::World& world) {
    auto renderer_ptr = world.get_resource<std::unique_ptr<renderer::Renderer>>();
    if (!renderer_ptr)
        return;
    auto& renderer = *renderer_ptr->get();

    auto game_res_opt = world.get_resource<GameResources>();
    if (!game_res_opt)
        return;
    auto& game_res = game_res_opt->get();

    // Livelier background color (Dark Navy Blue)
    core::ColorRGBA32F bg_color = core::ColorRGBA32F::rgba(0.08f, 0.12f, 0.2f, 1.0f);
    core::ColorRGB8 ui_bg_color = {20, 31, 51}; // Matching Text BG mapping

    renderer.submit(
        std::make_unique<renderer::halfblock::ClearColorCommand>(
            &renderer.resource_registry,
            game_res.color_texture,
            bg_color
        )
    );

    renderer::FrameBuffer<renderer::Attachment<core::ColorRGBA32F>> target {
        .attachments = {renderer::Attachment<core::ColorRGBA32F> {
            .view
            = {.texture = game_res.color_texture,
               .x       = 0,
               .y       = 0,
               .width   = game_res.screen_width,
               .height  = game_res.screen_height * 2},
            .load  = renderer::LoadOp::Load,
            .store = renderer::StoreOp::Store
        }}
    };

    for (auto [entity, transform, collider, render_color]:
         ecs::Query<core::Transform, physics::Collider, RenderColor>(&world))
    {
        Eigen::Matrix4f model      = transform.to_mat4();
        Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();
        projection(0, 0)           = 2.0f / 160.0f;
        projection(1, 1)           = 2.0f / 100.0f;

        renderer.submit(
            std::make_unique<QuadRenderCommand>(
                game_res.pipeline,
                target,
                game_res.quad_buffer,
                projection * model,
                render_color.color
            )
        );
    }

    renderer.submit(
        std::make_unique<renderer::halfblock::HalfBlockConversionCommand>(
            renderer.resource_registry,
            game_res.color_texture,
            game_res.char_texture
        )
    );

    // UI Render - Score & Lives
    if (auto score_opt = world.get_resource<Score>()) {
        renderer.submit(
            std::make_unique<TextRenderCommand>(
                renderer.resource_registry,
                game_res.char_texture,
                std::format("SCORE: {:05}", score_opt->get().value),
                5,
                1,
                core::ColorRGB8::YELLOW,
                ui_bg_color
            )
        );
    }
    if (auto lives_opt = world.get_resource<Lives>()) {
        renderer.submit(
            std::make_unique<TextRenderCommand>(
                renderer.resource_registry,
                game_res.char_texture,
                std::format("LIVES: {}", lives_opt->get().value),
                game_res.screen_width - 15,
                1,
                core::ColorRGB8::RED,
                ui_bg_color
            )
        );
    }

    // UI Render - Game Over Overlay
    if (auto status = world.get_resource<GameStatus>()) {
        if (status->get().state == GameState::GameOver) {
            std::string msg = "GAME OVER - PRESS SPACE TO RESTART";
            renderer.submit(
                std::make_unique<TextRenderCommand>(
                    renderer.resource_registry,
                    game_res.char_texture,
                    msg,
                    (game_res.screen_width / 2) - (msg.length() / 2),
                    game_res.screen_height / 2,
                    core::ColorRGB8::RED,
                    ui_bg_color
                )
            );
        }
    }

    renderer.render_frame();
}

///////////////////////////////////////////////// Main ///////////////////////////////////////////////////

int main() {
    auto app = std::make_shared<core::Application>();
    app->add_layer(core::input::InputLayer {});
    app->add_layer(physics::PhysicsLayer {});

    const std::uint32_t char_width  = 160;
    const std::uint32_t char_height = 50;

#if PLATFORM_WASM
    auto presenter = std::make_unique<web::BrowserPresenter>();
#else
    TerminalLayer term_layer(std::make_unique<Terminal>(), 60);
    app->world.insert_resource<TerminalLayer*>(&term_layer);
    app->add_layer(term_layer);
    auto presenter = term_layer.terminal->presenter();
#endif

    auto renderer
        = std::make_unique<renderer::Renderer>(char_width, char_height, std::move(presenter));
    auto color_texture_handle = renderer::halfblock::setup(*renderer, char_width, char_height);

    auto ui_pass = std::make_unique<renderer::RenderPass>("ui_pass");
    ui_pass->add_dependency(renderer::halfblock::HALFBLOCK_PASS);
    renderer->render_graph.add_pass(std::move(ui_pass));

    std::vector<Vertex> vertices
        = {{{-1.0f, -1.0f}},
           {{1.0f, -1.0f}},
           {{1.0f, 1.0f}},
           {{-1.0f, -1.0f}},
           {{1.0f, 1.0f}},
           {{-1.0f, 1.0f}}};
    auto quad_buffer = renderer->resource_registry.add_buffer<Vertex>(std::move(vertices)).value();

    auto shader = std::make_unique<QuadShader>();
    auto pipeline
        = renderer->resource_registry
              .add_pipeline(
                  std::make_unique<QuadPipeline>(renderer::PipelineDescriptor {}, std::move(shader))
              )
              .value();

    app->world.insert_resource(
        GameResources {
            .quad_buffer   = quad_buffer,
            .color_texture = color_texture_handle,
            .char_texture  = std::get<0>(renderer->render_target().attachments).view.texture,
            .pipeline      = pipeline,
            .screen_width  = char_width,
            .screen_height = char_height
        }
    );

    app->world.insert_resource(std::move(renderer));
    app->world.insert_resource(app.get());
    app->world.insert_resource(Score {0});
    app->world.insert_resource(Lives {5});
    app->world.insert_resource(GameStatus {GameState::Playing});

    // Spawn Player Paddle (Brighter Neon Green/Yellow)
    app->world.spawn(
        core::Transform::from_pos_rot_scale(
            {0.0f, -40.0f, 0.0f},
            Eigen::Quaternionf::Identity(),
            {15.0f, 2.5f, 1.0f}
        ),
        physics::Rigidbody {
            .linear_velocity = {0, 0},
            .mass            = 10.0f,
            .restitution     = 1.1f,
            .body_type       = physics::RigidbodyType::Kinematic
        },
        physics::Collider {
            .shape      = physics::AabbShape {{1.0f, 1.0f}},
            .filter     = {},
            .is_trigger = false
        },
        RenderColor {core::ColorRGBA32F::rgba(0.7f, 1.0f, 0.2f, 1.0f)},
        PaddleTag {}
    );

    spawn_ball(app->world);
    spawn_blocks(app->world);

    // Shrink the walls so they fit perfectly in the -80 to 80 viewport (-78 with an extent of 2 places their outer edge directly on 80)
    auto spawn_wall = [&](Eigen::Vector3f pos, Eigen::Vector2f extents) {
        app->world.spawn(
            core::Transform::from_pos_rot_scale(
                pos,
                Eigen::Quaternionf::Identity(),
                {extents.x(), extents.y(), 1.0f}
            ),
            physics::Rigidbody {
                .mass        = 0.0f,
                .restitution = 1.0f,
                .body_type   = physics::RigidbodyType::Static
            },
            physics::Collider {
                .shape      = physics::AabbShape {{1.0f, 1.0f}},
                .filter     = {},
                .is_trigger = false
            },
            RenderColor {core::ColorRGBA32F::rgba(0.85f, 0.9f, 0.9f, 1.0f)}
            // Bright Off-White/Light Cyan Walls
        );
    };

    spawn_wall({-78.0f, 0.0f, 0.0f}, {2.0f, 50.0f}); // Left
    spawn_wall({78.0f, 0.0f, 0.0f}, {2.0f, 50.0f}); // Right
    spawn_wall({0.0f, 48.0f, 0.0f}, {80.0f, 2.0f}); // Top

    app->scheduler.add_system(ecs::Stage::Update, paddle_system);
    app->scheduler.add_system(ecs::Stage::Update, ball_system);
    app->scheduler.add_system(ecs::Stage::Update, block_breaking_system);
    app->scheduler.add_system(ecs::Stage::PostUpdate, render_system);

    app->run();
    return 0;
}
