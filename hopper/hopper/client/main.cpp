#include <cmath>
#include <memory>

#include "renderer.hpp"

class UnlitShader: public renderer::Shader<float, float, float> {
public:
    float vertex(const float&, const renderer::ResourcePack<>&) override {
        return 0;
    }

    float fragment(const float&, const renderer::ResourcePack<>&) override {
        return 0;
    }

    ~UnlitShader() override = default;
};

using UnlitPipeline = renderer::Pipeline<float, float, float>;

int main() {
    renderer::Renderer app_renderer(160, 90);
    auto shader = std::make_unique<UnlitShader>();
    app_renderer.register_pipeline(std::make_unique<UnlitPipeline>(std::move(shader)));
    app_renderer.render_frame();
}
