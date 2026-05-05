#pragma once

#include <memory>
#include <string>

#include "renderer.hpp"
#include "renderer/command.hpp"
#include "renderer/graph.hpp"
#include "renderer/resource.hpp"

namespace renderer {

/**
 * The pipeline consists of:
 * 1. clear_pass - Clears the color buffer
 * 2. color_pass_bg - Renders background elements
 * 3. color_pass_fg - Renders foreground elements  
 * 4. pixel_pass - Converts color buffer to character pixels using halfblocks
 */
namespace halfblock {

    inline constexpr const char* CLEAR_PASS = "clear_pass";
    inline constexpr const char* COLOR_PASS_BG = "color_pass_bg";
    inline constexpr const char* COLOR_PASS_FG = "color_pass_fg";
    inline constexpr const char* HALFBLOCK_PASS = "halfblock_pass";

    // Implementation of clear command
    class ClearColorCommand: public RenderCommand {
    public:
        ClearColorCommand(
            ResourceRegistry* registry,
            TextureHandle<core::ColorRGBA32F> texture,
            core::ColorRGBA32F clear_color
        ):
            registry_(registry),
            texture_(texture),
            clear_color_(clear_color) {}

        const std::string target_pass() const override {
            return halfblock::CLEAR_PASS;
        }

        void execute(RenderPassEncoder&) override {
            if (auto tex = registry_->get_texture_mut(texture_); tex.has_value()) {
                auto& data = tex.value()->data_mut();
                std::fill(data.begin(), data.end(), clear_color_);
            }
        }

    private:
        ResourceRegistry* registry_;
        TextureHandle<core::ColorRGBA32F> texture_;
        core::ColorRGBA32F clear_color_;
    };

    /**
    * Converts the pixels to halfblock characters
    */
    class HalfBlockConversionCommand: public RenderCommand {
    public:
        HalfBlockConversionCommand(
            ResourceRegistry& registry,
            TextureHandle<core::ColorRGBA32F> color_texture,
            TextureHandle<CharacterPixel> char_texture
        ):
            registry_(registry),
            color_texture_(color_texture),
            char_texture_(char_texture) {}

        const std::string target_pass() const override {
            return halfblock::HALFBLOCK_PASS;
        }

        void execute(RenderPassEncoder&) override {
            auto color_tex = registry_.get_texture(color_texture_);
            auto char_tex = registry_.get_texture_mut(char_texture_);

            if (!color_tex || !char_tex)
                return;

            const auto* color_data = color_tex.value();
            auto* char_data = char_tex.value();

            const std::uint32_t color_width = color_data->width();
            const std::uint32_t color_height = color_data->height();
            const std::uint32_t char_width = char_data->width();
            const std::uint32_t char_height = char_data->height();

            auto to_rgb8 = [](const core::ColorRGBA32F& c) {
                return core::ColorRGB8::rgb(
                    static_cast<std::uint8_t>(std::clamp(c.r * 255.0f, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(c.g * 255.0f, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(c.b * 255.0f, 0.0f, 255.0f))
                );
            };

            for (std::uint32_t char_y = 0; char_y < char_height; ++char_y) {
                for (std::uint32_t char_x = 0; char_x < char_width; ++char_x) {
                    std::uint32_t color_x = std::min(char_x, color_width - 1);
                    std::uint32_t color_y_top = std::min(char_y * 2, color_height - 1);
                    std::uint32_t color_y_bottom = std::min(color_y_top + 1, color_height - 1);

                    core::ColorRGBA32F top
                        = color_data->data()[color_y_top * color_width + color_x];
                    core::ColorRGBA32F bottom
                        = color_data->data()[color_y_bottom * color_width + color_x];

                    char_data->data_mut()[char_y * char_width + char_x] = CharacterPixel {
                        .codepoint = U'▀',
                        .fg_color = to_rgb8(top),
                        .bg_color = to_rgb8(bottom)
                    };
                }
            }
        }

    private:
        ResourceRegistry& registry_;
        TextureHandle<core::ColorRGBA32F> color_texture_;
        TextureHandle<CharacterPixel> char_texture_;
    };

    // Inline implementations

    TextureHandle<core::ColorRGBA32F> inline setup(
        Renderer& renderer,
        std::uint32_t char_width,
        std::uint32_t char_height
    ) {
        const std::uint32_t pixel_width = char_width;
        const std::uint32_t pixel_height = char_height * 2;

        auto color_texture
            = renderer.resource_registry.add_texture<core::ColorRGBA32F>(pixel_width, pixel_height)
                  .value();

        // Set up the rendergraph
        renderer.render_graph.add_pass(std::make_unique<RenderPass>(halfblock::CLEAR_PASS));

        auto bg_pass = std::make_unique<RenderPass>(halfblock::COLOR_PASS_BG);
        bg_pass->add_dependency(halfblock::CLEAR_PASS);
        renderer.render_graph.add_pass(std::move(bg_pass));

        auto fg_pass = std::make_unique<RenderPass>(halfblock::COLOR_PASS_FG);
        fg_pass->add_dependency(halfblock::COLOR_PASS_BG);
        renderer.render_graph.add_pass(std::move(fg_pass));

        auto pixel_pass = std::make_unique<RenderPass>(halfblock::HALFBLOCK_PASS);
        pixel_pass->add_dependency(halfblock::COLOR_PASS_FG);
        renderer.render_graph.add_pass(std::move(pixel_pass));

        return color_texture;
    }

} // namespace halfblock
} // namespace renderer
