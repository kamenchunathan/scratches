#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "renderer/command.hpp"
#include "renderer/graph.hpp"
#include "renderer/present.hpp"
#include "renderer/resource.hpp"
#include "renderer/shader.hpp"

namespace renderer {

class Renderer {
public:
    // TODO: for testing
    PipelineRegistry pipeline_registry;
    BufferRegistry buffer_registry;
    ResourceRegistry resource_registry;
    RenderGraph render_graph;

    Renderer(std::uint32_t w, std::uint32_t h, std::unique_ptr<Presenter>);

    template<typename Pipeline>
    void register_pipeline(std::unique_ptr<Pipeline> pipeline);

    void submit(std::unique_ptr<RenderCommand>);

    void render_frame();

    BufferHandle<CharacterPixel> render_target_handle() const;
    BufferHandle<bool> mask_buffer_handle() const;

private:
    std::uint32_t viewport_width_, viewport_height_;
    std::unique_ptr<Presenter> presenter_;

    BufferHandle<CharacterPixel> front_buffer_;
    BufferHandle<CharacterPixel> back_buffer_;
    BufferHandle<bool> mask_buffer_;

    std::unordered_map<std::string, std::vector<std::unique_ptr<RenderCommand>>> command_queues_;

    void present();
    void swap_buffers();
};

template<typename Pipeline>
void Renderer::register_pipeline(std::unique_ptr<Pipeline> pipeline) {
    pipeline_registry.register_pipeline(std::move(pipeline));
}

} // namespace renderer
