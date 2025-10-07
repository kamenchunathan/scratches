#include <cmath>
#include <memory>
#include <print>
#include <thread>
#include <vector>

#include "color.hpp"
#include "renderer.hpp"
#include "renderer/buffer.hpp"
#include "renderer/graph.hpp"
#include "renderer/shader.hpp"
#include "renderer/types.hpp"
#include "term/layer.hpp"

using namespace std::chrono_literals;

struct ColorVertex {
    float x, y;
    core::ColorRGB8 character_color;
};

struct VOut {
    Eigen::Vector4f position;
    core::ColorRGB8 color;
};

class SimpleShader: public renderer::Shader<ColorVertex, VOut, renderer::CharacterPixel> {
public:
    VOut vertex(const ColorVertex& v, const renderer::ResourcePack<>&) override {
        Eigen::Vector4f clip_pos(v.x, v.y, 0.0f, 1.0f);
        return {clip_pos, v.character_color};
    }

    renderer::CharacterPixel fragment(const VOut& v, const renderer::ResourcePack<>&) override {
        return renderer::CharacterPixel {
            .codepoint = U'█',
            .fg_color = v.color,
            .bg_color = core::ColorRGB8::rgb(100, 0, 0)
        };
    }

    ~SimpleShader() override = default;
};

using UnlitPipeline = renderer::Pipeline<ColorVertex, VOut, renderer::CharacterPixel>;

class SimpleDrawCommand: public renderer::RenderCommand {
public:
    SimpleDrawCommand(
        renderer::BufferHandle<ColorVertex> vb,
        renderer::BufferHandle<renderer::CharacterPixel> ob,
        std::size_t count
    ):
        vb_(vb),
        ob_(ob),
        count_(count) {}

    const std::string target_pass() const override {
        return "main";
    }

    void execute(renderer::RenderPassEncoder& encoder) override {
        encoder.draw<ColorVertex, VOut, renderer::CharacterPixel>(vb_, ob_, count_, 0);
    }

private:
    renderer::BufferHandle<ColorVertex> vb_;
    renderer::BufferHandle<renderer::CharacterPixel> ob_;
    std::size_t count_;
};

int main() {
    const std::uint32_t width = 160;
    const std::uint32_t height = 45;

    auto term = Terminal();
    renderer::Renderer app_renderer(width, height, term.presenter());
    auto shader = std::make_unique<SimpleShader>();
    app_renderer.register_pipeline(
        std::make_unique<UnlitPipeline>(renderer::PipelineDescriptor {}, std::move(shader))
    );

    std::vector<ColorVertex> vertices = {
        {.x = 0.0f, .y = 0.8f, .character_color = core::ColorRGB8::rgb(255, 100, 200)},
        {.x = -0.8f, .y = -0.8f, .character_color = core::ColorRGB8::rgb(100, 100, 255)},
        {.x = 0.8f, .y = -0.8f, .character_color = core::ColorRGB8::rgb(100, 100, 255)}
    };

    auto output_buffer_handle = app_renderer.render_target_handle();
    auto vertex_buffer_handle =
        app_renderer.buffer_registry.create_buffer<ColorVertex>(width, height);

    auto* vertex_buffer = app_renderer.buffer_registry.get_buffer(vertex_buffer_handle);
    if (vertex_buffer) {
        vertex_buffer->update_buffer(vertices);
    }

    app_renderer.render_graph.add_pass(std::make_unique<renderer::RenderPass>("main"));
    app_renderer.submit(std::make_unique<SimpleDrawCommand>(
        vertex_buffer_handle,
        output_buffer_handle,
        vertices.size()
    ));
    app_renderer.render_frame();
    std::this_thread::sleep_for(5s);
}
