#include <cmath>
#include <cstdlib>
#include <format>
#include <iterator>
#include <memory>
#include <string_view>
#include <vector>

#include "application.hpp"
#include "color.hpp"
#include "ecs/query.hpp"
#include "ecs/system.hpp"
#include "ecs/world.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "renderer/command.hpp"
#include "renderer/graph.hpp"
#include "renderer/resource.hpp"
#include "renderer/shader.hpp"
#include "renderer/texture.hpp"
#include "renderer/types.hpp"
#include "term/layer.hpp"

//////////////////////////////////////////////////// Components //////////////////////////////////////////////////////

struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;
};

template<>
struct std::formatter<Transform>: public std::formatter<std::string_view> {
    auto format(const Transform& t, std::format_context& ctx) const {
        std::string tmp;
        std::format_to(std::back_inserter(tmp), "Transform {} {}", t.x, t.y);
        return std::formatter<std::string_view>::format(tmp, ctx);
    }
};

struct Triangle {
    core::ColorRGB8 color1;
    core::ColorRGB8 color2;
    core::ColorRGB8 color3;
};

//////////////////////////////////////////////////// Shaders //////////////////////////////////////////////////////

struct ColorVertex {
    float x, y;
    core::ColorRGBA32F color;
};

struct VOut {
    Eigen::Vector4f position;
    core::ColorRGBA32F color;
};

struct FragOut {
    core::ColorRGBA32F color;
};

class ColorShader: public renderer::ShaderPrev<ColorVertex, VOut, FragOut, Eigen::Vector2f> {
public:
    VOut vertex(const ColorVertex& v, const Eigen::Vector2f& position) override {
        Eigen::Vector4f clip_pos(v.x + position.x(), v.y + position.y(), 0.0f, 1.0f);
        return {clip_pos, v.color};
    }

    FragOut fragment(const VOut& v, const Eigen::Vector2f& position) override {
        (void)position;
        return {v.color};
    }
};

using ColorPipeline = renderer::Pipeline<ColorVertex, VOut, FragOut, Eigen::Vector2f>;

///////////////////////////////////////// ECS Resources for buffer handles //////////////////////////////////////////

struct VertexBufferResource {
    renderer::BufferHandle<ColorVertex> handle;
};

struct ColorTextureResource {
    renderer::TextureHandle<core::ColorRGBA32F> handle;
};

struct ColorPipelineResource {
    renderer::PipelineHandle<ColorPipeline> handle;
};

///////////////////////////////////////////////// Render commands /////////////////////////////////////////////////

class ColorPassCommand: public renderer::RenderCommand {
public:
    ColorPassCommand(
        renderer::PipelineHandle<ColorPipeline> pipeline_handle,
        renderer::FrameBuffer<renderer::Attachment<core::ColorRGBA32F>> target,
        renderer::BufferHandle<ColorVertex> vertex_buffer_handle,
        Eigen::Vector2f pos,
        std::uint32_t vertex_count
    ):
        pipeline_handle_(pipeline_handle),
        target_(target),
        vertex_buffer_handle_(vertex_buffer_handle),
        pos(pos),
        vertex_count_(vertex_count) {}

    const std::string target_pass() const override {
        return "color_pass";
    }

    void execute(renderer::RenderPassEncoder& encoder) override {
        encoder.draw(pipeline_handle_, target_, vertex_buffer_handle_, pos, vertex_count_);
    }

private:
    renderer::PipelineHandle<ColorPipeline> pipeline_handle_;
    renderer::FrameBuffer<renderer::Attachment<core::ColorRGBA32F>> target_;
    renderer::BufferHandle<ColorVertex> vertex_buffer_handle_;
    Eigen::Vector2f pos;
    std::uint32_t vertex_count_;
};

class HalfBlockCommand: public renderer::RenderCommand {
public:
    HalfBlockCommand(
        renderer::ResourceRegistry* resource_registry,
        renderer::TextureHandle<core::ColorRGBA32F> color_texture,
        renderer::TextureHandle<renderer::CharacterPixel> char_texture
    ):
        resource_registry_(resource_registry),
        color_texture_handle_(color_texture),
        char_texture_handle_(char_texture) {}

    const std::string target_pass() const override {
        return "pixel_pass";
    }

    void execute(renderer::RenderPassEncoder&) override {
        auto color_texture_exp = resource_registry_->get_texture(color_texture_handle_);
        auto char_texture_exp = resource_registry_->get_texture_mut(char_texture_handle_);

        if (!color_texture_exp || !char_texture_exp)
            return;

        const auto* color_texture = color_texture_exp.value();
        auto* char_texture = char_texture_exp.value();

        const std::uint32_t color_width = color_texture->width();
        const std::uint32_t color_height = color_texture->height();
        const std::uint32_t char_width = char_texture->width();
        const std::uint32_t char_height = char_texture->height();

        const auto& color_data = color_texture->data();
        auto& char_data = char_texture->data_mut();

        // Convert float colors to 8-bit
        auto to_rgb8 = [](const core::ColorRGBA32F& c) {
            return core::ColorRGB8::rgb(
                static_cast<uint8_t>(std::clamp(c.r * 255.0f, 0.0f, 255.0f)),
                static_cast<uint8_t>(std::clamp(c.g * 255.0f, 0.0f, 255.0f)),
                static_cast<uint8_t>(std::clamp(c.b * 255.0f, 0.0f, 255.0f))
            );
        };

        // Iterate through character buffer positions
        for (std::uint32_t char_y = 0; char_y < char_height; ++char_y) {
            for (std::uint32_t char_x = 0; char_x < char_width; ++char_x) {
                // Map character position to two color buffer pixels (top and bottom)
                std::uint32_t color_x = char_x;
                std::uint32_t color_y_top = char_y * 2;
                std::uint32_t color_y_bottom = color_y_top + 1;

                // Clamp to bounds
                color_x = std::min(color_x, color_width - 1);
                color_y_top = std::min(color_y_top, color_height - 1);
                color_y_bottom = std::min(color_y_bottom, color_height - 1);

                // Get the two pixels
                core::ColorRGBA32F top_color = color_data[color_y_top * color_width + color_x];
                core::ColorRGBA32F bottom_color =
                    color_data[color_y_bottom * color_width + color_x];

                // Create half-block character with appropriate colors
                char_data[char_y * char_width + char_x] = renderer::CharacterPixel {
                    .codepoint = U'▀',
                    .fg_color = to_rgb8(top_color),
                    .bg_color = to_rgb8(bottom_color)
                };
            }
        }
    }

private:
    renderer::ResourceRegistry* resource_registry_;
    renderer::TextureHandle<core::ColorRGBA32F> color_texture_handle_;
    renderer::TextureHandle<renderer::CharacterPixel> char_texture_handle_;
};

///////////////////////////////////////////////// Layers  /////////////////////////////////////////////////

class RendererLayer {
public:
    void build(core::Application& app) {
        // Get terminal dimensions
        const std::uint32_t char_width = 160;
        const std::uint32_t char_height = 45;
        const std::uint32_t pixel_width = 160;
        const std::uint32_t pixel_height = 90;

        // Create renderer
        auto* term_layer_ptr = app.world.get_resource<TerminalLayer*>();
        if (!term_layer_ptr || !*term_layer_ptr)
            return;

        auto renderer = std::make_unique<renderer::Renderer>(
            char_width,
            char_height,
            (*term_layer_ptr)->terminal->presenter()
        );

        // Add color pipeline
        auto color_shader = std::make_unique<ColorShader>();
        std::expected<renderer::PipelineHandle<ColorPipeline>, renderer::ResourceError>
            color_pipeline_handle_res =
                renderer->resource_registry.add_pipeline(std::make_unique<ColorPipeline>(
                    renderer::PipelineDescriptor {},
                    std::move(color_shader)
                ));

        if (!color_pipeline_handle_res) {
            // TODO: Better error handling
            std::exit(1);
        }

        renderer::PipelineHandle<ColorPipeline> color_pipeline_handle =
            color_pipeline_handle_res.value();

        // Create buffers and textures
        auto vertex_buffer_handle = renderer->resource_registry.add_buffer<ColorVertex>(3).value();
        auto color_texture_handle =
            renderer->resource_registry.add_texture<core::ColorRGBA32F>(pixel_width, pixel_height)
                .value();

        // Setup render graph
        renderer->render_graph.add_pass(std::make_unique<renderer::RenderPass>("color_pass"));
        auto pixel_pass = std::make_unique<renderer::RenderPass>("pixel_pass");
        pixel_pass->add_dependency("color_pass");
        renderer->render_graph.add_pass(std::move(pixel_pass));

        // Store buffer handles as ECS resources
        app.world.insert_resource(VertexBufferResource {vertex_buffer_handle});
        app.world.insert_resource(ColorTextureResource {color_texture_handle});
        app.world.insert_resource(ColorPipelineResource {color_pipeline_handle});

        // Store renderer
        app.world.insert_resource(std::move(renderer));
    }
};

///////////////////////////////////////////////// Systems  /////////////////////////////////////////////////

void input_system(ecs::World& world) {
    auto* input_state = world.get_resource<core::input::InputState>();
    if (!input_state)
        return;

    const float move_speed = 0.02f;

    for (auto [entity, transform]: ecs::Query<Transform>(&world)) {
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

        // Clamp position
        transform.x = std::clamp(transform.x, -1.5f, 1.5f);
        transform.y = std::clamp(transform.y, -1.5f, 1.5f);
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
    auto* renderer_ptr = world.get_resource<std::unique_ptr<renderer::Renderer>>();
    if (!renderer_ptr || !*renderer_ptr)
        return;
    auto* renderer = (*renderer_ptr).get();

    auto* color_pipeline_res = world.get_resource<ColorPipelineResource>();
    auto* vertex_buffer_res = world.get_resource<VertexBufferResource>();
    auto* color_texture_res = world.get_resource<ColorTextureResource>();
    if (!color_pipeline_res || !vertex_buffer_res || !color_texture_res)
        return;

    // TODO: Pass transform data to shader
    for (auto [entity, transform, triangle]: ecs::Query<Transform, Triangle>(&world)) {
        std::vector<ColorVertex> vertices = {
            {0.0f,
             0.4f,
             core::ColorRGBA32F::rgba(
                 triangle.color1.r / 255.0f,
                 triangle.color1.g / 255.0f,
                 triangle.color1.b / 255.0f,
                 1.0f
             )},
            {-0.4f,
             -0.4f,
             core::ColorRGBA32F::rgba(
                 triangle.color2.r / 255.0f,
                 triangle.color2.g / 255.0f,
                 triangle.color2.b / 255.0f,
                 1.0f
             )},
            {0.4f,
             -0.4f,
             core::ColorRGBA32F::rgba(
                 triangle.color3.r / 255.0f,
                 triangle.color3.g / 255.0f,
                 triangle.color3.b / 255.0f,
                 1.0f
             )}
        };

        if (std::expected<std::span<ColorVertex>, renderer::ResourceError> vb =
                renderer->resource_registry.get_buffer_mut(vertex_buffer_res->handle);
            vb.has_value())
        {
            assert(vb.value().size() == vertices.size() && "Mismatched sizes");
            std::copy(vertices.begin(), vertices.end(), vb.value().begin());
        }

        // Create the target for the color pass
        // TODO: Perhaps don't create this every time and maybe add frame buffer resource management
        renderer::FrameBuffer<renderer::Attachment<core::ColorRGBA32F>> color_pass_target {
            .attachments = {renderer::Attachment<core::ColorRGBA32F> {
                .view =
                    {.texture = color_texture_res->handle,
                     .x = 0,
                     .y = 0,
                     .widht = renderer->viewport_width,
                     .height = renderer->viewport_height * 2},
                .load = renderer::LoadOp::Clear,
                .store = renderer::StoreOp::Store,
            }}
        };

        // Submit render commands First pass: render triangle to color buffer
        renderer->submit(std::make_unique<ColorPassCommand>(
            color_pipeline_res->handle,
            color_pass_target,
            vertex_buffer_res->handle,
            Eigen::Vector2f(transform.x, transform.y),
            vertices.size()
        ));

        // Second pass: convert color buffer to character pixels
        auto final_target = renderer->render_target();
        auto final_target_handle = std::get<0>(final_target.attachments).view.texture;
        renderer->submit(std::make_unique<HalfBlockCommand>(
            &renderer->resource_registry,
            color_texture_res->handle,
            final_target_handle
        ));
    }

    renderer->render_frame();
}

int main() {
    core::Application app;

    app.add_layer(core::input::InputLayer {});
    TerminalLayer term_layer {.frame_rate = 60, .terminal = std::make_unique<Terminal>()};
    app.world.insert_resource<TerminalLayer*>(&term_layer);
    // NOTE: Workaround to allow the input system to send, the should_exist flag on the
    // application. Other fixes include writing an event system and using a resource,
    // will remove if I decide to do an event bus
    app.world.insert_resource<core::Application*>(&app);
    app.add_layer(term_layer);
    app.add_layer(RendererLayer {});

    // Create triangle entity
    app.world.spawn(
        Transform {0.0f, 0.0f, 0.0f},
        Triangle {
            core::ColorRGB8::rgb(255, 100, 200),
            core::ColorRGB8::rgb(100, 255, 100),
            core::ColorRGB8::rgb(100, 100, 255)
        }
    );

    app.scheduler.add_system(ecs::SystemStage::Update, input_system);
    app.scheduler.add_system(ecs::SystemStage::Update, render_system);

    app.run();
    return 0;
}
