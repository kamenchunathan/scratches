#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "renderer/buffer.hpp"
#include "renderer/command.hpp"
#include "renderer/graph.hpp"
#include "renderer/present.hpp"
#include "renderer/resource.hpp"
#include "renderer/shader.hpp"

#include "version.hpp"

namespace renderer {

class HOPPER_SINCE(0, 2) Renderer {
public:
    ResourceRegistry resource_registry;
    RenderGraph render_graph;

    std::uint32_t viewport_width, viewport_height;

    Renderer(std::uint32_t w, std::uint32_t h, std::unique_ptr<Presenter>);
    ~Renderer();

    FrameBuffer<Attachment<CharacterPixel>> render_target() const;

    void submit(std::unique_ptr<RenderCommand>);
    void render_frame();

private:
    std::unique_ptr<Presenter> presenter_;

    std::optional<FrameBuffer<Attachment<CharacterPixel>>> front_buffer_;
    std::optional<FrameBuffer<Attachment<CharacterPixel>>> back_buffer_;

    std::unordered_map<std::string, std::vector<std::unique_ptr<RenderCommand>>> command_queues_;

    void present();
    void swap_buffers();
};

class HOPPER_DEPRECATED(0, 2, "renderer::Renderer") RendererPrev {
public:
    // TODO: for testing
    PipelineRegistry pipeline_registry;
    BufferRegistry buffer_registry;
    ShaderResourceRegistry shader_resource_registry;
    RenderGraph render_graph;

    RendererPrev(std::uint32_t w, std::uint32_t h, std::unique_ptr<Presenter>);
    ~RendererPrev();

    template<typename Pipeline>
    void register_pipeline(std::unique_ptr<Pipeline> pipeline);

    void submit(std::unique_ptr<RenderCommandPrev>);

    void render_frame();

    BufferHandlePrev<CharacterPixel> render_target_handle() const;
    BufferHandlePrev<bool> mask_buffer_handle() const;

private:
    std::uint32_t viewport_width_, viewport_height_;
    std::unique_ptr<Presenter> presenter_;

    BufferHandlePrev<CharacterPixel> front_buffer_handle_;
    BufferHandlePrev<CharacterPixel> back_buffer_handle_;
    BufferHandlePrev<bool> mask_buffer_;

    std::unordered_map<std::string, std::vector<std::unique_ptr<RenderCommandPrev>>>
        command_queues_;

    void present();
    void swap_buffers();
};

template<typename Pipeline>
void RendererPrev::register_pipeline(std::unique_ptr<Pipeline> pipeline) {
    pipeline_registry.register_pipeline(std::move(pipeline));
}

} // namespace renderer
