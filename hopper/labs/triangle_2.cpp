#include <cmath>
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
#include "renderer/buffer.hpp"
#include "renderer/command.hpp"
#include "renderer/graph.hpp"
#include "renderer/shader.hpp"
#include "renderer/types.hpp"
#include "term/layer.hpp"

// Components
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

// Vertex structures
struct ColorVertex {
    float x, y;
    core::ColorRGBA32F color;
};

struct VOut {
    Eigen::Vector4f position;
    core::ColorRGBA32F color;
};

// First pass shader - renders to float color buffer
class ColorShader: public renderer::Shader<ColorVertex, VOut, core::ColorRGBA32F> {
public:
    VOut vertex(const ColorVertex& v, const renderer::ShaderResourcePack<>&) override {
        Eigen::Vector4f clip_pos(v.x, v.y, 0.0f, 1.0f);
        return {clip_pos, v.color};
    }

    core::ColorRGBA32F fragment(const VOut& v, const renderer::ShaderResourcePack<>&) override {
        return v.color;
    }
};

// ECS Resources for buffer handles
struct ColorBufferResource {
    renderer::BufferHandle<core::ColorRGBA32F> handle;
};

struct VertexBufferResource {
    renderer::BufferHandle<ColorVertex> handle;
};

struct CharacterBufferResource {
    renderer::BufferHandle<renderer::CharacterPixel> handle;
};

// Pipeline definition for first pass only
using ColorPipeline = renderer::Pipeline<ColorVertex, VOut, core::ColorRGBA32F>;

// Render commands
class ColorPassCommand: public renderer::RenderCommand {
public:
    ColorPassCommand(
        renderer::BufferHandle<ColorVertex> vb,
        renderer::BufferHandle<core::ColorRGBA32F> ob,
        std::size_t count
    ):
        vb_(vb),
        ob_(ob),
        count_(count) {}

    const std::string target_pass() const override {
        return "color_pass";
    }

    void execute(renderer::RenderPassEncoder& encoder) override {
        encoder.draw<ColorVertex, VOut, core::ColorRGBA32F>(vb_, ob_, count_, 0);
    }

private:
    renderer::BufferHandle<ColorVertex> vb_;
    renderer::BufferHandle<core::ColorRGBA32F> ob_;
    std::size_t count_;
};

// Second pass command - directly converts float buffer to character pixels
class PixelConversionCommand: public renderer::RenderCommand {
public:
    PixelConversionCommand(renderer::BufferRegistry* buffer_registry, ecs::World* world):
        buffer_registry_(buffer_registry),
        world_(world) {}

    const std::string target_pass() const override {
        return "pixel_pass";
    }

    void execute(renderer::RenderPassEncoder&) override {
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
                    .codepoint = U'▀', // Upper half block
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

        // Register shader for first pass only
        auto color_shader = std::make_unique<ColorShader>();
        renderer->register_pipeline(
            std::make_unique<ColorPipeline>(
                renderer::PipelineDescriptor {},
                std::move(color_shader)
            )
        );

        // Create buffers
        auto color_buffer =
            renderer->buffer_registry.create_buffer<core::ColorRGBA32F>(pixel_width, pixel_height);
        auto vertex_buffer =
            renderer->buffer_registry.create_buffer<ColorVertex>(pixel_width, pixel_height);
        auto char_buffer = renderer->render_target_handle();

        // Setup render graph
        renderer->render_graph.add_pass(std::make_unique<renderer::RenderPass>("color_pass"));
        auto pixel_pass = std::make_unique<renderer::RenderPass>("pixel_pass");
        pixel_pass->add_dependency("color_pass");
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
    auto* renderer = world.get_resource<std::unique_ptr<renderer::Renderer>>();
    if (!renderer || !*renderer)
        return;

    auto* color_buffer_res = world.get_resource<ColorBufferResource>();
    auto* vertex_buffer_res = world.get_resource<VertexBufferResource>();

    if (!color_buffer_res || !vertex_buffer_res)
        return;

    // Update triangle vertices based on transform
    for (auto [entity, transform, triangle]: ecs::Query<Transform, Triangle>(&world)) {
        std::vector<ColorVertex> vertices = {
            {transform.x + 0.0f,
             transform.y + 0.4f,
             core::ColorRGBA32F::rgba(
                 triangle.color1.r / 255.0f,
                 triangle.color1.g / 255.0f,
                 triangle.color1.b / 255.0f,
                 1.0f
             )},
            {transform.x - 0.4f,
             transform.y - 0.4f,
             core::ColorRGBA32F::rgba(
                 triangle.color2.r / 255.0f,
                 triangle.color2.g / 255.0f,
                 triangle.color2.b / 255.0f,
                 1.0f
             )},
            {transform.x + 0.4f,
             transform.y - 0.4f,
             core::ColorRGBA32F::rgba(
                 triangle.color3.r / 255.0f,
                 triangle.color3.g / 255.0f,
                 triangle.color3.b / 255.0f,
                 1.0f
             )}
        };

        if (auto* vb = (*renderer)->buffer_registry.get_buffer(vertex_buffer_res->handle)) {
            vb->update_buffer(vertices);
        }

        // Submit render commands
        // First pass: render triangle to color buffer
        (*renderer)->submit(
            std::make_unique<ColorPassCommand>(
                vertex_buffer_res->handle,
                color_buffer_res->handle,
                vertices.size()
            )
        );

        // Second pass: convert color buffer to character pixels
        (*renderer)->submit(
            std::make_unique<PixelConversionCommand>(&(*renderer)->buffer_registry, &world)
        );
    }

    (*renderer)->render_frame();
}

int main() {
    core::Application app;

    app.add_layer(core::input::InputLayer {});
    TerminalLayer term_layer {.frame_rate = 10, .terminal = std::make_unique<Terminal>()};
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
