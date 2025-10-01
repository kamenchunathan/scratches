#include "color.hpp"
#include "renderer.hpp"
#include <cmath>
#include <memory>

class UnlitShader: public renderer::Shader<float, float, float> {
public:
    ~UnlitShader() override = default;
    float vertex(const float&, const renderer::ResourcePack<>&) override {
        return 0;
    }

    float fragment(const float&, const renderer::ResourcePack<>&) override {
        return 0;
    }
};

using UnlitPipeline = renderer::Pipeline<float, float, float>;

int main() {
    renderer::Renderer app_renderer(160, 90);
    app_renderer.register_pipeline(
        std::make_unique<UnlitPipeline>(std::make_unique<UnlitShader>(), "unlit")
    );
    app_renderer.render_frame();
}
