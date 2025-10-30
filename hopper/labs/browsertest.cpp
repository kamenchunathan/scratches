#include "application.hpp"
#include "common.hpp"
#include "ecs/system.hpp"
#include "renderer.hpp"
#include "renderer/resource.hpp"
#include "renderer/types.hpp"
#include "time.hpp"

#if PLATFORM_WASM
    #include "web/layer.hpp"
    #include "web/presenter.hpp"
    #include <emscripten.h>
    #include <emscripten/html5.h>
#else
    #include "term/layer.hpp"
    #include "term/presenter.hpp"
#endif

struct Animation {
    std::vector<std::string> frames = {"|", "/", "-", "\\"};
    int current_frame = 0;
    float frame_duration = 0.1f; // seconds
    float time_since_last_frame = 0.0f;
};

class RendererLayer {
public:
    void build(std::shared_ptr<core::Application> app) {
        const std::uint32_t width = 80;
        const std::uint32_t height = 24;

#if PLATFORM_WASM
        auto renderer = std::make_unique<renderer::Renderer>(
            width,
            height,
            std::make_unique<web::BrowserPresenter>()
        );
#else
        auto renderer = std::make_unique<renderer::Renderer>(
            width,
            height,
            std::make_unique<term::Presenter>()
        );
#endif

        app->world.insert_resource(std::move(renderer));
    }
};

void animation_system(ecs::World& world) {
    auto* time = world.get_resource<core::Time>();
    if (!time)
        return;

    auto* animation = world.get_resource<Animation>();
    if (!animation)
        return;

    animation->time_since_last_frame += time->delta_seconds;
    if (animation->time_since_last_frame >= animation->frame_duration) {
        animation->time_since_last_frame -= animation->frame_duration;
        animation->current_frame = (animation->current_frame + 1) % animation->frames.size();
    }
}

void render_system(ecs::World& world) {
    auto* renderer_ptr = world.get_resource<std::unique_ptr<renderer::Renderer>>();
    if (!renderer_ptr || !*renderer_ptr)
        return;
    renderer::Renderer& renderer = **renderer_ptr;

    auto* animation = world.get_resource<Animation>();
    if (!animation)
        return;

    renderer::FrameBuffer<renderer::Attachment<renderer::CharacterPixel>> render_target =
        renderer.render_target();
    renderer::TextureHandle<renderer::CharacterPixel> render_texture_handle =
        std::get<0>(render_target.attachments).view.texture;

    renderer::Texture2D<renderer::CharacterPixel>* render_texture = nullptr;
    if (auto maybe_render_texture =
            renderer.resource_registry.get_texture_mut(render_texture_handle);
        maybe_render_texture.has_value())
    {
        render_texture = maybe_render_texture.value();
    } else {
        return;
    }

    // Clear buffer
    std::fill(
        render_texture->data_mut().begin(),
        render_texture->data_mut().end(),
        renderer::CharacterPixel {}
    );

    // Draw animation
    const std::string& frame = animation->frames[animation->current_frame];
    if (!frame.empty()) {
        int x = render_texture->width() / 2;
        int y = render_texture->height() / 2;
        render_texture->data_mut()[y * render_texture->width() + x] = {
            .codepoint = static_cast<char32_t>(frame[0]),
            .fg_color = core::ColorRGB8::WHITE,
            .bg_color = core::ColorRGB8::BLACK,
        };
    }

    renderer.render_frame();
}

int main() {
    // Application has to be static because it's methods and variables are accessed
    // in a callback from javascript
    static auto app = std::make_shared<core::Application>();

#if PLATFORM_WASM
    app->add_layer(BrowserLayer {});
#else
    app->add_layer(TerminalLayer {});
#endif
    app->add_layer(RendererLayer {});

    app->world.insert_resource(Animation {});

    app->scheduler.add_system(ecs::SystemStage::Update, animation_system);
    app->scheduler.add_system(ecs::SystemStage::Update, render_system);

    app->run();
}
