#include <cctype>
#include <cmath>
#include <format>
#include <iterator>
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#include "application.hpp"
#include "color.hpp"
#include "ecs/query.hpp"
#include "ecs/system.hpp"
#include "ecs/world.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "renderer/buffer.hpp"
#include "renderer/command.hpp"
#include "renderer/graph.hpp"
#include "renderer/shader.hpp"
#include "renderer/texture.hpp"
#include "renderer/types.hpp"
#include "term/layer.hpp"

// Components
struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;
    float scale = 1.0f;
};

template<>
struct std::formatter<Transform>: public std::formatter<std::string_view> {
    auto format(const Transform& t, std::format_context& ctx) const {
        std::string tmp;
        std::format_to(std::back_inserter(tmp), "Transform {} {} Scale: {}", t.x, t.y, t.scale);
        return std::formatter<std::string_view>::format(tmp, ctx);
    }
};

struct SpriteComponent {
    std::shared_ptr<renderer::Texture2D<core::ColorRGBA8>> texture;
};

struct BackgroundTag {};

// Vertex structures
struct TexturedVertex {
    float x, y;
    float u, v;
};

struct VOut {
    Eigen::Vector4f position;
    Eigen::Vector2f tex_coord;
};

struct DiffuseTextureTag {};
using DiffuseTextureBinding = renderer::Binding<core::ColorRGBA8, DiffuseTextureTag>;

class TextureShader:
    public renderer::ShaderPrev<TexturedVertex, VOut, core::ColorRGBA32F, DiffuseTextureBinding> {
public:
    VOut vertex(const TexturedVertex& v, const DiffuseTextureBinding&) override {
        Eigen::Vector4f clip_pos(v.x, v.y, 0.0f, 1.0f);
        return {clip_pos, {v.u, v.v}};
    }

    core::ColorRGBA32F fragment(const VOut& v, const DiffuseTextureBinding& texture) override {
        renderer::Sampler<core::ColorRGBA8> sampler;
        core::ColorRGBA8 sampled_color = sampler.sample(
            renderer::FilterMode::Nearest,
            renderer::WrapMode::Repeat,
            texture.inner,
            v.tex_coord.x(),
            v.tex_coord.y()
        );
        return core::ColorRGBA32F::rgba(
            sampled_color.r / 255.0f,
            sampled_color.g / 255.0f,
            sampled_color.b / 255.0f,
            sampled_color.a / 255.0f
        );
    }
};

// ------------------------------ ECS Resources for buffer handles ----------------------------------------
struct ColorBufferResource {
    renderer::BufferHandlePrev<core::ColorRGBA32F> handle;
};

struct VertexBufferResource {
    renderer::BufferHandlePrev<TexturedVertex> handle;
};

struct CharacterBufferResource {
    renderer::BufferHandlePrev<renderer::CharacterPixel> handle;
};

// ------------------------------------- Pipeline definitions ----------------------------------------------
using TexturePipeline =
    renderer::PipelinePrev<TexturedVertex, VOut, core::ColorRGBA32F, DiffuseTextureBinding>;

// ------------------------------------- Render Commands  ----------------------------------------------
class ColorPassCommand: public renderer::RenderCommandPrev {
public:
    ColorPassCommand(
        renderer::BufferHandlePrev<TexturedVertex> vb,
        renderer::BufferHandlePrev<core::ColorRGBA32F> ob,
        std::vector<TexturedVertex> vertices,
        std::string pass_name,
        renderer::ShaderResourceRegistry* resource_registry,
        renderer::BufferRegistry* buffer_registry,
        std::shared_ptr<renderer::Texture2D<core::ColorRGBA8>> texture
    ):
        vb_(vb),
        ob_(ob),
        vertices_(std::move(vertices)),
        pass_name_(std::move(pass_name)),
        resource_registry_(resource_registry),
        buffer_registry_(buffer_registry),
        texture_(texture) {}

    const std::string target_pass() const override {
        return pass_name_;
    }

    void execute(renderer::RenderPassEncoderPrev& encoder) override {
        if (auto* vb = buffer_registry_->get_buffer(vb_)) {
            vb->update_buffer(vertices_);
        }

        resource_registry_->bind<DiffuseTextureBinding>(DiffuseTextureBinding {*texture_});
        encoder.draw_prev<TexturedVertex, VOut, core::ColorRGBA32F, DiffuseTextureBinding>(
            vb_,
            ob_,
            vertices_.size(),
            0
        );
    }

private:
    renderer::BufferHandlePrev<TexturedVertex> vb_;
    renderer::BufferHandlePrev<core::ColorRGBA32F> ob_;
    std::vector<TexturedVertex> vertices_;
    std::string pass_name_;
    renderer::ShaderResourceRegistry* resource_registry_;
    renderer::BufferRegistry* buffer_registry_;
    std::shared_ptr<renderer::Texture2D<core::ColorRGBA8>> texture_;
};

class ClearCommand: public renderer::RenderCommandPrev {
public:
    ClearCommand(
        renderer::BufferHandlePrev<core::ColorRGBA32F> ob,
        renderer::BufferRegistry* buffer_registry,
        core::ColorRGBA32F clear_color = {0.0f, 0.0f, 0.0f, 1.0f}
    ):
        ob_(ob),
        buffer_registry_(buffer_registry),
        clear_color_(clear_color) {}

    const std::string target_pass() const override {
        return "clear_pass";
    }

    void execute(renderer::RenderPassEncoderPrev&) override {
        if (auto* buffer = buffer_registry_->get_buffer(ob_)) {
            std::fill(buffer->data().begin(), buffer->data().end(), clear_color_);
        }
    }

private:
    renderer::BufferHandlePrev<core::ColorRGBA32F> ob_;
    renderer::BufferRegistry* buffer_registry_;
    core::ColorRGBA32F clear_color_;
};

/* Converts pixels into halfblock characters */
class HalfBlockCommand: public renderer::RenderCommandPrev {
public:
    HalfBlockCommand(renderer::BufferRegistry* buffer_registry, ecs::World* world):
        buffer_registry_(buffer_registry),
        world_(world) {}

    const std::string target_pass() const override {
        return "pixel_pass";
    }

    void execute(renderer::RenderPassEncoderPrev&) override {
        // Get buffer handles from ECS resources
        auto* color_buffer_res = world_->get_resource<ColorBufferResource>();
        auto* char_buffer_res = world_->get_resource<CharacterBufferResource>();

        if (!color_buffer_res || !char_buffer_res)
            return;

        auto* color_buffer = buffer_registry_->get_buffer(color_buffer_res->handle);
        auto* char_buffer = buffer_registry_->get_buffer(char_buffer_res->handle);

        if (!color_buffer || !char_buffer)
            return;

        const std::uint32_t color_width = color_buffer->width();
        const std::uint32_t color_height = color_buffer->height();
        const std::uint32_t char_width = char_buffer->width();
        const std::uint32_t char_height = char_buffer->height();

        const auto& color_data = color_buffer->data();
        std::vector<renderer::CharacterPixel> char_data(char_width * char_height);

        // Convert float colors to 8-bit
        auto to_rgb8 = [](const core::ColorRGBA32F& c) {
            return core::ColorRGB8::rgb(
                static_cast<uint8_t>(std::clamp(c.r * 255.0f, 0.0f, 255.0f)),
                static_cast<uint8_t>(std::clamp(c.g * 255.0f, 0.0f, 255.0f)),
                static_cast<uint8_t>(std::clamp(c.b * 255.0f, 0.0f, 255.0f))
            );
        };

        for (std::uint32_t char_y = 0; char_y < char_height; ++char_y) {
            for (std::uint32_t char_x = 0; char_x < char_width; ++char_x) {
                std::uint32_t color_x = char_x;
                std::uint32_t color_y_top = char_y * 2;
                std::uint32_t color_y_bottom = color_y_top + 1;

                color_x = std::min(color_x, color_width - 1);
                color_y_top = std::min(color_y_top, color_height - 1);
                color_y_bottom = std::min(color_y_bottom, color_height - 1);

                core::ColorRGBA32F top_color = color_data[color_y_top * color_width + color_x];
                core::ColorRGBA32F bottom_color =
                    color_data[color_y_bottom * color_width + color_x];

                char_data[char_y * char_width + char_x] = renderer::CharacterPixel {
                    .codepoint = U'▀',
                    .fg_color = to_rgb8(top_color),
                    .bg_color = to_rgb8(bottom_color)
                };
            }
        }

        char_buffer->update_buffer(char_data);
    }

private:
    renderer::BufferRegistry* buffer_registry_;
    ecs::World* world_;
};

// Renderer Layer
class RendererLayer {
public:
    void build(core::Application& app) {
        const std::uint32_t char_width = 160;
        const std::uint32_t char_height = 45;
        const std::uint32_t pixel_width = 160;
        const std::uint32_t pixel_height = 90;

        auto* term_layer_ptr = app.world.get_resource<TerminalLayer*>();
        if (!term_layer_ptr || !*term_layer_ptr)
            return;

        auto renderer = std::make_unique<renderer::RendererPrev>(
            char_width,
            char_height,
            (*term_layer_ptr)->terminal->presenter()
        );

        renderer->register_pipeline(
            std::make_unique<TexturePipeline>(
                renderer::PipelineDescriptor {},
                std::make_unique<TextureShader>()
            )
        );

        auto color_buffer =
            renderer->buffer_registry.create_buffer<core::ColorRGBA32F>(pixel_width, pixel_height);
        auto vertex_buffer =
            renderer->buffer_registry.create_buffer<TexturedVertex>(pixel_width, pixel_height);
        auto char_buffer = renderer->render_target_handle();

        renderer->render_graph.add_pass(std::make_unique<renderer::RenderPass>("clear_pass"));
        auto color_pass_bg = std::make_unique<renderer::RenderPass>("color_pass_bg");
        color_pass_bg->add_dependency("clear_pass");
        renderer->render_graph.add_pass(std::move(color_pass_bg));
        auto color_pass_fg = std::make_unique<renderer::RenderPass>("color_pass_fg");
        color_pass_fg->add_dependency("color_pass_bg");
        renderer->render_graph.add_pass(std::move(color_pass_fg));
        auto pixel_pass = std::make_unique<renderer::RenderPass>("pixel_pass");
        pixel_pass->add_dependency("color_pass_fg");
        renderer->render_graph.add_pass(std::move(pixel_pass));

        // Store buffer handles as ECS resources
        app.world.insert_resource(ColorBufferResource {color_buffer});
        app.world.insert_resource(VertexBufferResource {vertex_buffer});
        app.world.insert_resource(CharacterBufferResource {char_buffer});

        // Store renderer
        app.world.insert_resource(std::move(renderer));
    }
};

// Systems
void input_system(ecs::World& world) {
    auto* input_state = world.get_resource<core::input::InputState>();
    if (!input_state)
        return;

    const float move_speed = 0.1f;
    const float scale_speed = 0.02f;

    for (auto [entity, transform]: ecs::Query<Transform>(&world).without<BackgroundTag>()) {
        if (input_state->is_button_down(core::input::KeyCode::W)
            || input_state->is_button_down(core::input::KeyCode::Up))
        {
            transform.y += move_speed;
        }
        if (input_state->is_button_down(core::input::KeyCode::S)
            || input_state->is_button_down(core::input::KeyCode::Down))
        {
            transform.y -= move_speed;
        }
        if (input_state->is_button_down(core::input::KeyCode::A)
            || input_state->is_button_down(core::input::KeyCode::Left))
        {
            transform.x -= move_speed;
        }
        if (input_state->is_button_down(core::input::KeyCode::D)
            || input_state->is_button_down(core::input::KeyCode::Right))
        {
            transform.x += move_speed;
        }

        if (input_state->is_button_down(core::input::KeyCode::I)) {
            transform.scale += scale_speed;
        }
        if (input_state->is_button_down(core::input::KeyCode::K)) {
            transform.scale -= scale_speed;
        }

        // Clamp position
        transform.x = std::clamp(transform.x, -0.5f, 0.5f);
        transform.y = std::clamp(transform.y, -0.5f, 0.5f);
        transform.scale = std::clamp(transform.scale, 0.1f, 2.0f);
    }

    if (input_state->just_pressed(core::input::KeyCode::Q)
        || input_state->just_pressed(core::input::KeyCode::Escape))
    {
        if (auto* app_ptr = world.get_resource<core::Application*>()) {
            (*app_ptr)->set_should_exit(true);
        }
    }
}

void render_system(ecs::World& world) {
    auto* renderer_ptr = world.get_resource<std::unique_ptr<renderer::RendererPrev>>();
    if (!renderer_ptr || !*renderer_ptr)
        return;
    auto& renderer = **renderer_ptr;

    auto* color_buffer_res = world.get_resource<ColorBufferResource>();
    auto* vertex_buffer_res = world.get_resource<VertexBufferResource>();

    if (!color_buffer_res || !vertex_buffer_res)
        return;

    renderer.submit(
        std::make_unique<ClearCommand>(color_buffer_res->handle, &renderer.buffer_registry)
    );

    // Draw background
    for (auto [entity, transform, sprite, tag]:
         ecs::Query<Transform, SpriteComponent, BackgroundTag>(&world))
    {
        std::vector<TexturedVertex> vertices = {
            {-1.0f, -1.0f, 0.0f, 1.0f}, // bottom-left
            {1.0f, -1.0f, 1.0f, 1.0f}, // bottom-right
            {-1.0f, 1.0f, 0.0f, 0.0f}, // top-left

            {1.0f, -1.0f, 1.0f, 1.0f}, // bottom-right
            {1.0f, 1.0f, 1.0f, 0.0f}, // top-right
            {-1.0f, 1.0f, 0.0f, 0.0f}, // top-left
        };

        renderer.submit(
            std::make_unique<ColorPassCommand>(
                vertex_buffer_res->handle,
                color_buffer_res->handle,
                std::move(vertices),
                "color_pass_bg",
                &renderer.shader_resource_registry,
                &renderer.buffer_registry,
                sprite.texture
            )
        );
    }

    // Update quad vertices based on transform
    for (auto [entity, transform, sprite]:
         ecs::Query<Transform, SpriteComponent>(&world).without<BackgroundTag>())
    {
        const float half_width = 0.5f * transform.scale;
        const float half_height = 0.5f * transform.scale;

        std::vector<TexturedVertex> vertices = {
            {transform.x - half_width, transform.y - half_height, 0.0f, 1.0f}, // bottom-left
            {transform.x + half_width, transform.y - half_height, 1.0f, 1.0f}, // bottom-right
            {transform.x - half_width, transform.y + half_height, 0.0f, 0.0f}, // top-left

            {transform.x + half_width, transform.y - half_height, 1.0f, 1.0f}, // bottom-right
            {transform.x + half_width, transform.y + half_height, 1.0f, 0.0f}, // top-right
            {transform.x - half_width, transform.y + half_height, 0.0f, 0.0f}, // top-left
        };

        renderer.submit(
            std::make_unique<ColorPassCommand>(
                vertex_buffer_res->handle,
                color_buffer_res->handle,
                std::move(vertices),
                "color_pass_fg",
                &renderer.shader_resource_registry,
                &renderer.buffer_registry,
                sprite.texture
            )
        );
    }

    renderer.submit(std::make_unique<HalfBlockCommand>(&renderer.buffer_registry, &world));

    renderer.render_frame();
}

int main() {
    core::Application app;

    app.add_layer(core::input::InputLayer {});
    TerminalLayer term_layer {.frame_rate = 60, .terminal = std::make_unique<Terminal>()};
    app.world.insert_resource<TerminalLayer*>(&term_layer);
    app.world.insert_resource<core::Application*>(&app);
    app.add_layer(term_layer);
    app.add_layer(RendererLayer {});

    auto bg_tex_opt = renderer::Texture2D<core::ColorRGBA8>::load_png("./assets/background.png");
    if (!bg_tex_opt.has_value())
        return 1;
    auto bg_tex = std::make_shared<renderer::Texture2D<core::ColorRGBA8>>(std::move(*bg_tex_opt));
    app.world.spawn(Transform {0.0f, 0.0f, 0.0f, 1.0f}, SpriteComponent {bg_tex}, BackgroundTag {});

    auto tex_opt = renderer::Texture2D<core::ColorRGBA8>::load_png("./assets/bingo.png");
    if (!tex_opt.has_value())
        return 1;
    auto tex = std::make_shared<renderer::Texture2D<core::ColorRGBA8>>(std::move(*tex_opt));
    app.world.spawn(Transform {0.0f, 0.0f, 0.0f, 0.5f}, SpriteComponent {tex});

    app.scheduler.add_system(ecs::SystemStage::Update, input_system);
    app.scheduler.add_system(ecs::SystemStage::Update, render_system);

    app.run();
    return 0;
}
