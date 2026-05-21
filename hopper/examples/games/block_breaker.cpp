#include <algorithm>
#include <codecvt>
#include <format>
#include <fstream>
#include <memory>
#include <vector>

#include <Eigen/Dense>

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

#include <random>

// --- Themes ---

struct GameTheme {
    std::string name;
    core::ColorRGBA32F background;
    core::ColorRGB8 ui_background;
    core::ColorRGB8 contrast_background;
    core::ColorRGBA32F paddle;
    core::ColorRGBA32F ball;
    core::ColorRGBA32F walls;
    std::vector<core::ColorRGBA32F> block_rows;
    core::ColorRGB8 title_color;
};

const std::vector<GameTheme> THEMES = {
    {"Nord",
     core::ColorRGBA32F::rgba(0.18f, 0.20f, 0.25f, 1.0f), // nord0
     {46, 52, 64},                                       // nord0
     {59, 66, 82},                                       // nord1
     core::ColorRGBA32F::rgba(0.53f, 0.75f, 0.82f, 1.0f), // nord8 (Blue)
     core::ColorRGBA32F::rgba(0.93f, 0.94f, 0.96f, 1.0f), // nord4
     core::ColorRGBA32F::rgba(0.85f, 0.87f, 0.91f, 1.0f), // nord5
     {
         core::ColorRGBA32F::rgba(0.75f, 0.38f, 0.42f, 1.0f), // nord11 (Red)
         core::ColorRGBA32F::rgba(0.82f, 0.53f, 0.44f, 1.0f), // nord12 (Orange)
         core::ColorRGBA32F::rgba(0.92f, 0.79f, 0.55f, 1.0f), // nord13 (Yellow)
         core::ColorRGBA32F::rgba(0.64f, 0.75f, 0.55f, 1.0f)  // nord14 (Green)
     },
     {136, 192, 208}}, // nord8
    {"Catppuccin",
     core::ColorRGBA32F::rgba(0.11f, 0.11f, 0.18f, 1.0f), // crust
     {30, 30, 46},                                       // base
     {17, 17, 27},                                       // mantle
     core::ColorRGBA32F::rgba(0.99f, 0.70f, 0.84f, 1.0f), // pink
     core::ColorRGBA32F::rgba(0.80f, 0.95f, 0.95f, 1.0f), // sky
     core::ColorRGBA32F::rgba(0.71f, 0.76f, 0.94f, 1.0f), // lavender
     {
         core::ColorRGBA32F::rgba(0.96f, 0.63f, 0.69f, 1.0f), // red
         core::ColorRGBA32F::rgba(1.00f, 0.78f, 0.60f, 1.0f), // peach
         core::ColorRGBA32F::rgba(0.98f, 0.89f, 0.70f, 1.0f), // yellow
         core::ColorRGBA32F::rgba(0.65f, 0.89f, 0.63f, 1.0f)  // green
     },
     {203, 166, 247}}, // mauve
    {"Cyberpunk",
     core::ColorRGBA32F::rgba(0.05f, 0.05f, 0.10f, 1.0f), // Deep Black/Purple
     {10, 10, 20},                                       // Dark UI
     {30, 0, 40},                                        // Neon Purple contrast
     core::ColorRGBA32F::rgba(0.0f, 1.0f, 1.0f, 1.0f),    // Cyan Paddle
     core::ColorRGBA32F::rgba(1.0f, 1.0f, 0.0f, 1.0f),    // Yellow Ball
     core::ColorRGBA32F::rgba(1.0f, 0.0f, 1.0f, 1.0f),    // Magenta Walls
     {
         core::ColorRGBA32F::rgba(1.0f, 0.0f, 0.5f, 1.0f), // Hot Pink
         core::ColorRGBA32F::rgba(0.5f, 0.0f, 1.0f, 1.0f), // Purple
         core::ColorRGBA32F::rgba(0.0f, 0.5f, 1.0f, 1.0f), // Blue
         core::ColorRGBA32F::rgba(0.0f, 1.0f, 0.5f, 1.0f)  // Green
     },
     {0, 255, 255}} // Cyan
};

// --- Game State & Components ---

enum class GameState { StartScreen, Playing, GameOver, GameWon, HighScoreInput, HighScoreScreen };

struct GameStatus {
    GameState state = GameState::StartScreen;
};

struct Streak {
    int chain            = 0;
    int frames_remaining = 0; // Timer based on frames (assuming ~60 frames per second)

    int get_multiplier() const {
        return 1 + (chain / 5); // +1 multiplier for every 5 blocks broken in succession
    }
};

struct HighScoreEntry {
    std::string name;
    int score;
};

struct HighScores {
    std::vector<HighScoreEntry> entries;
    std::string current_input = "";
};

void load_high_scores(HighScores& hs) {
    hs.entries.clear();
#ifndef PLATFORM_WASM
    std::ifstream file("highscores.dat");
    std::string name;
    int score;
    while (file >> name >> score) {
        hs.entries.push_back({name, score});
    }
#endif
    while (hs.entries.size() < 5)
        hs.entries.push_back({"---", 0});
    std::sort(hs.entries.begin(), hs.entries.end(), [](const auto& a, const auto& b) {
        return a.score > b.score;
    });
}

void save_high_scores(const HighScores& hs) {
#ifndef PLATFORM_WASM
    std::ofstream file("highscores.dat");
    for (const auto& entry: hs.entries) {
        file << entry.name << " " << entry.score << "\n";
    }
#endif
}

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
        std::u32string text,
        std::uint32_t x,
        std::uint32_t y,
        core::ColorRGB8 fg = core::ColorRGB8::WHITE,
        core::ColorRGB8 bg = core::ColorRGB8::BLACK
    ):
        registry_(registry),
        target_texture_(target_texture),
        text_(std::move(text)),
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
                = {.codepoint = text_[i], .fg_color = fg_, .bg_color = bg_};
        }
    }

private:
    renderer::ResourceRegistry& registry_;
    renderer::TextureHandle<renderer::CharacterPixel> target_texture_;
    std::u32string text_;
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

    auto theme_opt = world.get_resource<GameTheme>();
    if (!theme_opt)
        return;
    const auto& theme = theme_opt->get();

    // Adjusted play area size so blocks comfortably fit inside the new visible walls
    for (int y = 0; y < 4; ++y) {
        core::ColorRGBA32F color = (y < theme.block_rows.size()) ? theme.block_rows[y] : core::ColorRGBA32F::WHITE;
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
                RenderColor {color},
                BlockTag {}
            );
        }
    }
}

void spawn_ball(ecs::World& world) {
    // Clear existing balls
    std::vector<ecs::Entity> to_delete;
    for (auto [e, t]: ecs::Query<BallTag>(&world)) {
        to_delete.push_back(e);
    }
    for (auto e: to_delete)
        world.destroy(e);

    auto theme_opt = world.get_resource<GameTheme>();
    core::ColorRGBA32F ball_color = theme_opt ? theme_opt->get().ball : core::ColorRGBA32F::WHITE;

    world.spawn(
        core::Transform::from_pos_rot_scale(
            {0.0f, 0.0f, 0.0f},
            Eigen::Quaternionf::Identity(),
            {2.0f, 2.0f, 1.0f}
        ),
        physics::Rigidbody {
            .linear_velocity = {35.0f, 40.0f},
            .mass            = 1.0f,
            .restitution     = 1.1f, // Slight boost for liveliness
            .body_type       = physics::RigidbodyType::Dynamic
        },
        physics::Collider {
            .shape      = physics::AabbShape {{1.0f, 1.0f}},
            .filter     = {},
            .is_trigger = false
        },
        RenderColor {ball_color},
        BallTag {}
    );
}

// --- Systems ---

/// Helper for keyboard input parsing
char get_char_pressed(const core::input::InputState& input) {
    for (std::uint32_t i = static_cast<int>(core::input::KeyCode::A);
         i <= static_cast<int>(core::input::KeyCode::Z);
         ++i)
    {
        if (input.just_pressed(static_cast<core::input::KeyCode>(i))) {
            return 'A' + (i - static_cast<int>(core::input::KeyCode::A));
        }
    }
    return 0;
}

std::u32string to_u32(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
    return convert.from_bytes(str);
}

void game_flow_system(ecs::World& world) {
    auto status_opt = world.get_resource<GameStatus>();
    auto input_opt  = world.get_resource<core::input::InputState>();
    if (!status_opt || !input_opt)
        return;

    auto& status = status_opt->get();
    auto& input  = input_opt->get();

    // App Quit / Back to Menu
    if (input.just_pressed(core::input::KeyCode::Escape)
        || input.just_pressed(core::input::KeyCode::Q))
    {
        if (status.state == GameState::HighScoreScreen || status.state == GameState::HighScoreInput)
        {
            status.state = GameState::StartScreen; // Back to start
            if (auto hs_opt = world.get_resource<HighScores>())
                hs_opt->get().current_input = "";
        } else if (auto app = world.get_resource<core::Application*>()) {
            app->get()->set_should_exit(true);
        }
    }

    if (status.state == GameState::StartScreen) {
        if (input.just_pressed(core::input::KeyCode::Space) || input.just_pressed(core::input::KeyCode::Enter)) {
            status.state                              = GameState::Playing;
            world.get_resource<Score>()->get().value  = 0;
            world.get_resource<Lives>()->get().value  = 5;
            world.get_resource<Streak>()->get().chain = 0;
            spawn_blocks(world);
            spawn_ball(world);

            // Reset Paddle Position
            for (auto [e, transform]: ecs::Query<core::Transform>(&world).with<PaddleTag>()) {
                transform.translation = {0.0f, -40.0f, 0.0f};
            }
        } else if (input.just_pressed(core::input::KeyCode::H)) {
            status.state = GameState::HighScoreScreen;
        }
    } else if (status.state == GameState::GameOver || status.state == GameState::GameWon) {
        if (input.just_pressed(core::input::KeyCode::Space) || input.just_pressed(core::input::KeyCode::Enter)) {
            status.state = GameState::StartScreen; // Return to title screen
        }
    } else if (status.state == GameState::HighScoreInput) {
        auto hs_opt = world.get_resource<HighScores>();
        if (!hs_opt)
            return;
        auto& hs = hs_opt->get();

        char c = get_char_pressed(input);
        if (c != 0 && hs.current_input.length() < 3) {
            hs.current_input += c;
        }
        if ((input.just_pressed(core::input::KeyCode::Backspace)
             || input.just_pressed(core::input::KeyCode::Delete))
            && !hs.current_input.empty())
        {
            hs.current_input.pop_back();
        }
        if (input.just_pressed(core::input::KeyCode::Enter) && hs.current_input.length() == 3) {
            int final_score = world.get_resource<Score>()->get().value;
            hs.entries.push_back({hs.current_input, final_score});
            std::sort(hs.entries.begin(), hs.entries.end(), [](const auto& a, const auto& b) {
                return a.score > b.score;
            });
            hs.entries.pop_back(); // Remove the lowest (6th) entry
            save_high_scores(hs);

            hs.current_input = "";
            status.state     = GameState::HighScoreScreen;
        }
    }
}


void streak_timer_system(ecs::World& world) {
    auto status = world.get_resource<GameStatus>();
    if (status && status->get().state != GameState::Playing)
        return;

    if (auto streak_res = world.get_resource<Streak>()) {
        auto& streak = streak_res->get();
        if (streak.frames_remaining > 0) {
            streak.frames_remaining--;
            if (streak.frames_remaining == 0) {
                streak.chain = 0; // Streak timed out!
            }
        }
    }
}

void ball_system(ecs::World& world) {
    auto status_opt = world.get_resource<GameStatus>();
    if (!status_opt)
        return;
    auto& status = status_opt->get();

    if (status.state != GameState::Playing)
        return;

    std::vector<ecs::Entity> balls_to_destroy;
    auto score_val = world.get_resource<Score>()->get().value;
    auto hs_opt    = world.get_resource<HighScores>();

    // Check Win Condition
    int blocks_remaining = 0;
    for (auto _: ecs::Query<BlockTag>(&world))
        blocks_remaining++;

    if (blocks_remaining == 0) {
        for (auto [e, _]: ecs::Query<BallTag>(&world))
            balls_to_destroy.push_back(e);
        if (hs_opt && score_val > hs_opt->get().entries.back().score) {
            status.state = GameState::HighScoreInput;
        } else {
            status.state = GameState::GameWon;
        }
    }

    // Ball bounds checks & life tracking
    for (auto [entity, transform, rb]:
         ecs::Query<core::Transform, physics::Rigidbody>(&world).with<BallTag>())
    {
        if (transform.translation.y() < -55.0f) {
            if (auto lives_opt = world.get_resource<Lives>()) {
                lives_opt->get().value -= 1;

                // Reset streak on life loss
                world.get_resource<Streak>()->get().chain            = 0;
                world.get_resource<Streak>()->get().frames_remaining = 0;

                if (lives_opt->get().value <= 0) {
                    balls_to_destroy.push_back(entity);
                    if (hs_opt && score_val > hs_opt->get().entries.back().score) {
                        status.state = GameState::HighScoreInput;
                    } else {
                        status.state = GameState::GameOver;
                    }
                } else {
                    transform.translation = {0.0f, 0.0f, 0.0f};
                    rb.linear_velocity    = {35.0f, 40.0f};
                }
            }
        } else {
            // Speed Clamping (Keep existing logic)
            float speed = rb.linear_velocity.norm();
            if (speed > 0.1f) {
                if (speed < 40.0f)
                    rb.linear_velocity = rb.linear_velocity.normalized() * 40.0f;
                else if (speed > 80.0f)
                    rb.linear_velocity = rb.linear_velocity.normalized() * 80.0f;
            }
        }
    }

    for (auto e: balls_to_destroy)
        world.destroy(e);
}

void block_breaking_system(ecs::World& world) {
    auto status = world.get_resource<GameStatus>();
    if (status && status->get().state != GameState::Playing)
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

        auto streak_opt = world.get_resource<Streak>();
        auto score_opt  = world.get_resource<Score>();

        if (streak_opt && score_opt) {
            auto& streak = streak_opt->get();
            streak.chain += blocks_to_destroy.size();
            streak.frames_remaining = 180; // Reset timer to ~3 seconds at 60Hz

            int score_gained
                = static_cast<int>(blocks_to_destroy.size()) * 100 * streak.get_multiplier();
            score_opt->get().value += score_gained;
        }

        for (auto e: blocks_to_destroy)
            world.destroy(e);
    }
}

void paddle_system(ecs::World& world) {
    if (auto status = world.get_resource<GameStatus>()) {
        if (status->get().state != GameState::Playing)
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
        rb.linear_velocity.x() = move * 120.0f;

        // Shrink the clamping zone to account for the newly visible walls (walls are at +/-76.0f inner boundary)
        if (transform.translation.x() < -61.0f && rb.linear_velocity.x() < 0)
            rb.linear_velocity.x() = 0;
        if (transform.translation.x() > 61.0f && rb.linear_velocity.x() > 0)
            rb.linear_velocity.x() = 0;
    }
}

void render_system(ecs::World& world) {
    auto renderer_ptr = world.get_resource<std::unique_ptr<renderer::Renderer>>();
    auto game_res_opt = world.get_resource<GameResources>();
    auto status_opt   = world.get_resource<GameStatus>();
    auto theme_opt    = world.get_resource<GameTheme>();
    if (!renderer_ptr || !game_res_opt || !status_opt || !theme_opt)
        return;

    auto& renderer = *renderer_ptr->get();
    auto& game_res = game_res_opt->get();
    auto state     = status_opt->get().state;
    const auto& theme = theme_opt->get();

    core::ColorRGBA32F bg_color = theme.background;
    core::ColorRGB8 ui_bg_color = theme.ui_background;
    core::ColorRGB8 contrast_bg = theme.contrast_background;

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

    // Draw world objects only if playing, won, or game over
    if (state == GameState::Playing || state == GameState::GameOver || state == GameState::GameWon)
    {
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
    }

    renderer.submit(
        std::make_unique<renderer::halfblock::HalfBlockConversionCommand>(
            renderer.resource_registry,
            game_res.color_texture,
            game_res.char_texture
        )
    );

    // --- UI RENDERING ---
    auto render_text
        = [&](std::u32string msg, int x, int y, core::ColorRGB8 fg, core::ColorRGB8 bg) {
              renderer.submit(
                  std::make_unique<TextRenderCommand>(
                      renderer.resource_registry,
                      game_res.char_texture,
                      msg,
                      x,
                      y,
                      fg,
                      bg
                  )
              );
          };

    if (state == GameState::StartScreen) {
        std::u32string title = U" N E O N   B R E A K E R ";
        render_text(
            title,
            (game_res.screen_width / 2) - (title.length() / 2),
            20,
            theme.title_color,
            ui_bg_color
        );
        render_text(
            to_u32(std::format("Theme: {}", theme.name)),
            (game_res.screen_width / 2) - (theme.name.length() + 7) / 2,
            22,
            core::ColorRGB8::WHITE,
            ui_bg_color
        );
        render_text(
            U"Press SPACE to Start",
            (game_res.screen_width / 2) - 10,
            26,
            core::ColorRGB8::YELLOW,
            ui_bg_color
        );
        render_text(
            U"Press H for High Scores",
            (game_res.screen_width / 2) - 11,
            26,
            core::ColorRGB8::WHITE,
            ui_bg_color
        );
        render_text(
            U"Press ESC or Q to Quit",
            (game_res.screen_width / 2) - 11,
            28,
            core::ColorRGB8::WHITE,
            ui_bg_color
        );
        render_text(
            U"Built with Hopper Engine (c) 2026",
            (game_res.screen_width / 2) - 16,
            45,
            core::ColorRGB8::MAGENTA,
            ui_bg_color
        );
    } else if (state == GameState::HighScoreScreen) {
        render_text(
            U" --- HIGH SCORES --- ",
            (game_res.screen_width / 2) - 10,
            15,
            core::ColorRGB8::YELLOW,
            ui_bg_color
        );
        auto hs   = world.get_resource<HighScores>()->get();
        int y_pos = 20;
        for (const auto& entry: hs.entries) {
            auto hs_str = to_u32(std::format("{} ...... {:05}", entry.name, entry.score));
            render_text(
                hs_str,
                (game_res.screen_width / 2) - (hs_str.length() / 2),
                y_pos,
                core::ColorRGB8::WHITE,
                ui_bg_color
            );
            y_pos += 2;
        }
        render_text(
            U"Press ESC to Return",
            (game_res.screen_width / 2) - 9,
            35,
            core::ColorRGB8::CYAN,
            ui_bg_color
        );
    } else if (state == GameState::HighScoreInput) {
        render_text(
            U" NEW HIGH SCORE! ",
            (game_res.screen_width / 2) - 8,
            18,
            core::ColorRGB8::GREEN,
            ui_bg_color
        );

        auto score_val = world.get_resource<Score>()->get().value;
        auto score_str = to_u32(std::format("SCORE: {:05}", score_val));
        render_text(
            score_str,
            (game_res.screen_width / 2) - (score_str.length() / 2),
            20,
            core::ColorRGB8::YELLOW,
            ui_bg_color
        );

        render_text(
            U"Enter 3 Initials: ",
            (game_res.screen_width / 2) - 9,
            24,
            core::ColorRGB8::WHITE,
            ui_bg_color
        );

        std::u32string input_str
            = to_u32(world.get_resource<HighScores>()->get().current_input + "_");
        render_text(
            input_str,
            (game_res.screen_width / 2) - 1,
            26,
            core::ColorRGB8::CYAN,
            ui_bg_color
        );
    } else if (
        state == GameState::Playing || state == GameState::GameOver || state == GameState::GameWon
    )
    {
        // Background Bar for Top UI
        std::u32string ui_bar(game_res.screen_width, U' ');
        render_text(ui_bar, 0, 0, core::ColorRGB8::WHITE, contrast_bg);
        render_text(ui_bar, 0, 1, core::ColorRGB8::WHITE, contrast_bg);

        // SCORE
        auto score_val = world.get_resource<Score>()->get().value;
        render_text(
            to_u32(std::format("SCORE: {:05}", score_val)),
            5,
            1,
            core::ColorRGB8::YELLOW,
            contrast_bg
        );

        // STREAK
        auto streak = world.get_resource<Streak>()->get();
        if (streak.chain > 0) {
            std::u32string streak_txt = to_u32(
                std::format(
                    "STREAK: {}x ({} left)",
                    streak.get_multiplier(),
                    streak.frames_remaining / 60
                )
            );
            render_text(
                streak_txt,
                (game_res.screen_width / 2) - (streak_txt.length() / 2),
                1,
                core::ColorRGB8::MAGENTA,
                contrast_bg
            );
        }

        // LIVES (HEARTS)
        auto lives_val = world.get_resource<Lives>()->get().value;
        std::u32string hearts;
        for (int i = 0; i < lives_val; ++i)
            hearts += U"♥ ";
        render_text(
            to_u32("LIVES: ") + hearts,
            game_res.screen_width - 25,
            1,
            core::ColorRGB8::RED,
            contrast_bg
        );

        // OVERLAYS
        if (state == GameState::GameOver) {
            std::u32string msg = U" GAME OVER - PRESS SPACE TO CONTINUE ";
            render_text(
                msg,
                (game_res.screen_width / 2) - (msg.length() / 2),
                game_res.screen_height / 2,
                core::ColorRGB8::WHITE,
                core::ColorRGB8::RED
            );
        } else if (state == GameState::GameWon) {
            std::u32string msg = U" YOU WIN! - PRESS SPACE TO CONTINUE ";
            render_text(
                msg,
                (game_res.screen_width / 2) - (msg.length() / 2),
                game_res.screen_height / 2,
                core::ColorRGB8::WHITE,
                core::ColorRGB8::GREEN
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

    app->world.insert_resource(Streak {0, 0});

    // Random Theme Selection
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, THEMES.size() - 1);
    const GameTheme& selected_theme = THEMES[dis(gen)];
    app->world.insert_resource(selected_theme);

    HighScores initial_hs;
    load_high_scores(initial_hs);
    app->world.insert_resource(initial_hs);
    app->world.insert_resource(std::move(renderer));
    app->world.insert_resource(app.get());
    app->world.insert_resource(Score {0});
    app->world.insert_resource(Lives {5});
    app->world.insert_resource(GameStatus {GameState::StartScreen});

    // Spawn Player Paddle
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
        RenderColor {selected_theme.paddle},
        PaddleTag {}
    );

    // Note: Ball and Blocks are spawned by game_flow_system when transitioning to Playing state

    // Walls
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
            RenderColor {selected_theme.walls}
        );
    };

    spawn_wall({-78.0f, 0.0f, 0.0f}, {2.0f, 50.0f}); // Left
    spawn_wall({78.0f, 0.0f, 0.0f}, {2.0f, 50.0f}); // Right
    spawn_wall({0.0f, 48.0f, 0.0f}, {80.0f, 2.0f}); // Top

    app->scheduler.add_system(ecs::Stage::PreUpdate, game_flow_system);
    app->scheduler.add_system(ecs::Stage::PreUpdate, streak_timer_system);
    app->scheduler.add_system(ecs::Stage::Update, paddle_system);
    app->scheduler.add_system(ecs::Stage::Update, ball_system);
    app->scheduler.add_system(ecs::Stage::Update, block_breaking_system);
    app->scheduler.add_system(ecs::Stage::PostUpdate, render_system);

    app->run();
    return 0;
}
