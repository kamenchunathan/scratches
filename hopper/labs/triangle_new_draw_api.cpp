#include <format>
#include <iterator>
#include <memory>
#include <print>
#include <string_view>
#include <vector>

#include "application.hpp"
#include "color.hpp"
#include "common.hpp"
#include "ecs/query.hpp"
#include "ecs/system.hpp"
#include "ecs/world.hpp"
#include "input.hpp"
#include "log.hpp"
#include "log/layer.hpp"
#include "renderer.hpp"
#include "renderer/command.hpp"
#include "renderer/graphs/halfblock.hpp"
#include "renderer/resource.hpp"
#include "renderer/shader.hpp"
#include "renderer/types.hpp"

#if PLATFORM_WASM
    #include "web/layer.hpp"
    #include "web/presenter.hpp"
#else
    #include "term/layer.hpp"
#endif

//////////////////////////////////////////////////// Components //////////////////////////////////////////////////////

struct Transform {
    float x        = 0.0f;
    float y        = 0.0f;
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

class ColorShader:
    public renderer::ShaderPrev<ColorVertex, VOut, core::ColorRGBA32F, Eigen::Vector2f> {
public:
    VOut vertex(const ColorVertex& v, const Eigen::Vector2f& position) override {
        Eigen::Vector4f clip_pos(v.x + position.x(), v.y + position.y(), 0.0f, 1.0f);
        return {clip_pos, v.color};
    }

    core::ColorRGBA32F fragment(const VOut& v, const Eigen::Vector2f& position) override {
        (void)position;
        return v.color;
    }
};

using ColorPipeline = renderer::Pipeline<ColorVertex, VOut, core::ColorRGBA32F, Eigen::Vector2f>;

////////////////////////////////////////////// ECS Resources ///////////////////////////////////////////////

struct RenderResources {
    renderer::BufferHandle<ColorVertex> vertex_buffer;
    renderer::TextureHandle<core::ColorRGBA32F> color_texture;
    renderer::TextureHandle<renderer::CharacterPixel> char_texture;
    renderer::PipelineHandle<ColorPipeline> pipeline;
};

/////////////////////////////////////////// Render commands /////////////////////////////////////////////////

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
        pos_(pos),
        vertex_count_(vertex_count) {}

    const std::string target_pass() const override {
        return renderer::halfblock::COLOR_PASS_FG;
    }

    void execute(renderer::RenderPassEncoder& encoder) override {
        HOPPER_TRACE(
            "renderer",
            "ColorPassCommand::execute: {} vertices at ({}, {})",
            vertex_count_,
            pos_.x(),
            pos_.y()
        );
        encoder.draw(pipeline_handle_, target_, vertex_buffer_handle_, pos_, vertex_count_);
    }

private:
    renderer::PipelineHandle<ColorPipeline> pipeline_handle_;
    renderer::FrameBuffer<renderer::Attachment<core::ColorRGBA32F>> target_;
    renderer::BufferHandle<ColorVertex> vertex_buffer_handle_;
    Eigen::Vector2f pos_;
    std::uint32_t vertex_count_;
};

///////////////////////////////////////////////// Layers  /////////////////////////////////////////////////

class RendererLayer {
public:
    void build(std::shared_ptr<core::Application> app) {
        const std::uint32_t char_width  = 160;
        const std::uint32_t char_height = 45;

        HOPPER_INFO("renderer", "Setting up renderer ({}x{})", char_width, char_height);

#if PLATFORM_WASM
        auto presenter = std::make_unique<web::BrowserPresenter>();
#else
        auto term_layer_ptr = app->world.get_resource<TerminalLayer*>();
        if (!term_layer_ptr || !*term_layer_ptr)
            return;
        auto presenter = term_layer_ptr->get()->terminal->presenter();
#endif

        auto renderer
            = std::make_unique<renderer::Renderer>(char_width, char_height, std::move(presenter));
        HOPPER_INFO("renderer", "Renderer created");

        // Set up halfblock render graph
        auto color_texture_handle = renderer::halfblock::setup(*renderer, char_width, char_height);
        HOPPER_DEBUG("renderer", "Halfblock graph setup complete");

        // Create pipeline
        auto color_shader    = std::make_unique<ColorShader>();
        auto pipeline_handle = //
            renderer->resource_registry
                .add_pipeline(
                    std::make_unique<ColorPipeline>(
                        renderer::PipelineDescriptor {},
                        std::move(color_shader)
                    )
                )
                .value();
        HOPPER_INFO("renderer", "Color pipeline created");

        // Create vertex buffer
        auto vertex_buffer = renderer->resource_registry.add_buffer<ColorVertex>(3).value();
        HOPPER_DEBUG("renderer", "Vertex buffer allocated (3 vertices)");

        // Store resources
        app->world.insert_resource(
            RenderResources {
                .vertex_buffer = vertex_buffer,
                .color_texture = color_texture_handle,
                .char_texture  = std::get<0>(renderer->render_target().attachments).view.texture,
                .pipeline      = pipeline_handle
            }
        );
        HOPPER_DEBUG("renderer", "Render resources stored in world");

        app->world.insert_resource(std::move(renderer));

        app->scheduler.add_system(
            ecs::Stage::Update,
            [](ecs::World& world) {
                HOPPER_TRACE("renderer", "Halfblock conversion system running");
                auto renderer_ptr = world.get_resource<std::unique_ptr<renderer::Renderer>>();
                if (!renderer_ptr)
                    return;
                auto renderer = renderer_ptr->get().get();

                auto resources_opt = world.get_resource<RenderResources>();
                if (!resources_opt)
                    return;
                auto resources = resources_opt->get();

                renderer->submit(
                    std::make_unique<renderer::halfblock::HalfBlockConversionCommand>(
                        renderer->resource_registry,
                        resources.color_texture,
                        std::get<0>(renderer->render_target().attachments).view.texture
                    )
                );
            }

        );
        HOPPER_DEBUG("renderer", "Halfblock conversion system registered");
    }
};

///////////////////////////////////////////////// Systems  /////////////////////////////////////////////////

void input_system(ecs::World& world) {
    auto input_state = world.get_resource<core::input::InputState>();
    if (!input_state)
        return;

    const float move_speed = 0.02f;

    auto query = ecs::Query<Transform>(&world);
    auto it    = query.begin();
    if (it != query.end()) {
        auto [entity, transform] = *it;
        if (input_state->get().is_button_down(core::input::KeyCode::W)
            || input_state->get().is_button_down(core::input::KeyCode::Up))
        {
            transform.y += move_speed;
        }
        if (input_state->get().is_button_down(core::input::KeyCode::S)
            || input_state->get().is_button_down(core::input::KeyCode::Down))
        {
            transform.y -= move_speed;
        }
        if (input_state->get().is_button_down(core::input::KeyCode::A)
            || input_state->get().is_button_down(core::input::KeyCode::Left))
        {
            transform.x -= move_speed;
        }
        if (input_state->get().is_button_down(core::input::KeyCode::D)
            || input_state->get().is_button_down(core::input::KeyCode::Right))
        {
            transform.x += move_speed;
        }

        transform.x = std::clamp(transform.x, -1.5f, 1.5f);
        transform.y = std::clamp(transform.y, -1.5f, 1.5f);
    }

    if (input_state->get().just_pressed(core::input::KeyCode::Q)
        || input_state->get().just_pressed(core::input::KeyCode::Escape))
    {
        HOPPER_INFO("input", "Quit requested");
        if (auto app_ptr = world.get_resource<core::Application*>()) {
            app_ptr->get()->set_should_exit(true);
        }
    }
}

void render_system(ecs::World& world) {
    HOPPER_TRACE("renderer", "render_system: starting frame");
    auto renderer_ptr = world.get_resource<std::unique_ptr<renderer::Renderer>>();
    if (!renderer_ptr)
        return;
    auto renderer      = renderer_ptr->get().get();
    auto resources_opt = world.get_resource<RenderResources>();
    if (!resources_opt)
        return;
    auto resources = resources_opt->get();

    // Submit clear command
    renderer->submit(
        std::make_unique<renderer::halfblock::ClearColorCommand>(
            &renderer->resource_registry,
            resources.color_texture,
            core::ColorRGBA32F::BLACK
        )
    );
    HOPPER_TRACE("renderer", "Clear command submitted");

    // Render triangle
    auto query = ecs::Query<Transform, Triangle>(&world);
    auto it    = query.begin();
    if (it != query.end()) {
        auto [entity, transform, triangle] = *it;
        std::vector<ColorVertex> vertices
            = {{0.0f,
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
                )}};

        // Update vertex buffer
        if (auto vb = renderer->resource_registry.get_buffer_mut(resources.vertex_buffer);
            vb.has_value())
        {
            std::copy(vertices.begin(), vertices.end(), vb.value().begin());
        }
        HOPPER_TRACE("renderer", "Vertex buffer updated with triangle vertices");

        // Create render target
        renderer::FrameBuffer<renderer::Attachment<core::ColorRGBA32F>> target {
            .attachments = {renderer::Attachment<core::ColorRGBA32F> {
                .view =
                    {
                        .texture = resources.color_texture,
                        .x = 0,
                        .y = 0,
                        .width = renderer->viewport_width,
                        .height = renderer->viewport_height * 2,
                    },
                .load = renderer::LoadOp::Load,
                .store = renderer::StoreOp::Store
            }}
        };

        renderer->submit(
            std::make_unique<ColorPassCommand>(
                resources.pipeline,
                target,
                resources.vertex_buffer,
                Eigen::Vector2f(transform.x, transform.y),
                vertices.size()
            )
        );
        HOPPER_DEBUG("renderer", "Triangle draw submitted at ({}, {})", transform.x, transform.y);
    }

    renderer->render_frame();
    HOPPER_TRACE("renderer", "render_system: frame complete");
}

int main() {
    static auto app = std::make_shared<core::Application>();

    app->add_layer(logging::LogLayer {});
    app->add_layer(core::input::InputLayer {});

#if PLATFORM_WASM
    web::BrowserLayer browser_layer;
    app->add_layer(browser_layer);
#else
    TerminalLayer term_layer(std::make_unique<Terminal>(), 60);
    app->world.insert_resource<TerminalLayer*>(&term_layer);
    app->add_layer(term_layer);
#endif

    app->world.insert_resource<core::Application*>(app.get());
    app->add_layer(RendererLayer {});

    app->world.spawn(
        Transform {0.0f, 0.0f, 0.0f},
        Triangle {
            core::ColorRGB8::rgb(255, 100, 200),
            core::ColorRGB8::rgb(100, 255, 100),
            core::ColorRGB8::rgb(100, 100, 255)
        }
    );

    app->scheduler.add_system(ecs::Stage::PreUpdate, input_system);
    app->scheduler.add_system(ecs::Stage::PostUpdate, render_system);

    app->run();
    return 0;
}
