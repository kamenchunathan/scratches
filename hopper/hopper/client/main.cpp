#include <cmath>
#include <memory>
#include <print>
#include <thread>
#include <vector>

#include "color.hpp"
#include "renderer.hpp"
#include "renderer/buffer.hpp"
#include "renderer/graph.hpp"
#include "renderer/present/term.hpp"
#include "renderer/types.hpp"

using namespace std::chrono_literals;

struct ColorVertex {
    float x, y;
    core::ColorRGB8 color;
};

class UnlitShader: public renderer::Shader<ColorVertex, ColorVertex, renderer::CharacterPixel> {
public:
    ColorVertex vertex(const ColorVertex& v, const renderer::ResourcePack<>&) override {
        // Pass-through shader
        return v;
    }

    renderer::CharacterPixel
    fragment(const ColorVertex& v, const renderer::ResourcePack<>&) override {
        return renderer::CharacterPixel { .codepoint = U'█',
                                          .fg_color = v.color,
                                          .bg_color = core::ColorRGB8::rgb(0, 0, 0) };
    }

    ~UnlitShader() override = default;
};

using UnlitPipeline = renderer::Pipeline<ColorVertex, ColorVertex, renderer::CharacterPixel>;

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
        encoder.draw<ColorVertex, ColorVertex, renderer::CharacterPixel>(
            vb_,
            ob_,
            count_, // Draw all vertices
            0 // first vertex
        );
    }

private:
    renderer::BufferHandle<ColorVertex> vb_;
    renderer::BufferHandle<renderer::CharacterPixel> ob_;
    std::size_t count_;
};

int main() {
    const std::uint32_t width = 160;
    const std::uint32_t height = 45;

    auto term = renderer::Terminal();
    renderer::Renderer app_renderer(width, height, term.presenter());
    auto shader = std::make_unique<UnlitShader>();
    app_renderer.register_pipeline(std::make_unique<UnlitPipeline>(std::move(shader)));

    // Generate a vertex for each pixel on the canvas to create a gradient
    std::vector<ColorVertex> vertices;
    vertices.reserve(width * height);
    for (std::uint32_t j = 0; j < height; ++j) {
        for (std::uint32_t i = 0; i < width; ++i) {
            float u = static_cast<float>(i) / (width - 1);
            float v = static_cast<float>(j) / (height - 1);
            vertices.push_back({
                .x = u * 2.0f - 1.0f,
                .y = v * 2.0f - 1.0f,
                .color = core::ColorRGB8::rgb(
                    static_cast<uint8_t>(u * 255),
                    static_cast<uint8_t>(v * 255),
                    0
                ),
            });
        }
    }

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
