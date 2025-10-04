#include <cmath>
#include <memory>

#include "color.hpp"
#include "renderer.hpp"
#include "renderer/buffer.hpp"
#include "renderer/types.hpp"

struct ColorVertex {
    float x, y;
    core::ColorRGB8 color;
};

class UnlitShader: public renderer::Shader<ColorVertex, ColorVertex, renderer::CharacterPixel> {
public:
    ColorVertex vertex(const ColorVertex& v, const renderer::ResourcePack<>&) override {
        // Assume normalized coordinates
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
        renderer::BufferHandle<renderer::CharacterPixel> ob
    ):
        vb_(vb),
        ob_(ob) {}

    const std::string target_pass() const override {
        return "main";
    }

    void execute(renderer::RenderPassEncoder& encoder) override {
        encoder.draw<ColorVertex, ColorVertex, renderer::CharacterPixel>(
            vb_,
            ob_,
            3, // vertex count
            0 // first vertex
        );
    }

private:
    renderer::BufferHandle<ColorVertex> vb_;
    renderer::BufferHandle<renderer::CharacterPixel> ob_;
};

int main() {
    renderer::Renderer app_renderer(160, 90);
    auto shader = std::make_unique<UnlitShader>();
    app_renderer.register_pipeline(std::make_unique<UnlitPipeline>(std::move(shader)));

    std::vector<ColorVertex> vertices = { { -0.5f, -0.5f, { 255, 0, 0 } },
                                          { 0.5f, -0.5f, { 0, 255, 0 } },
                                          { 0.0f, 0.5f, { 0, 0, 255 } } };

    auto output_buffer = app_renderer.front_buffer_;
    // auto vertex_buffer = app_renderer.buffer_registry.create_buffer<ColorVertex>(
    //     1,
    //     static_cast<std::uint32_t>(vertices.size())
    // );
    //
    // app_renderer.submit(std::make_unique<SimpleDrawCommand>(vertex_buffer, output_buffer));
    // app_renderer.render_frame();
}
