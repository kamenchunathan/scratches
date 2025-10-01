#pragma once

#include <cstdint>
#include <vector>

#include "renderer/shader.hpp"

namespace renderer {

class RenderPassEncoder {
public:
    template<typename Pipeline>
    void draw(std::uint32_t vertex_count, std::uint32_t first_vertex = 0) {
        // NOTE: requires the following to be available where it is instantiated:
        // - Full definition of renderer::Renderer
        // - Full definition of renderer::Pipeline and its traits (VertexInType, get_shader())
        // - A resource system on the renderer that can provide a vertex buffer.

        auto* pipeline = pipeline_registry_.get_pipeline<Pipeline>();
        if (!pipeline) {
            return;
        }

        auto* shader = pipeline->get_shader();
        if (!shader) {
            return;
        }

        // TODO: Complete implementation
    }

    void draw_indexed(
        const std::vector<uint32_t>& indices,
        uint32_t first_index = 0,
        uint32_t count = 0
    );

private:
    friend class Renderer;

    RenderPassEncoder(PipelineRegistry& pipeline_registry): pipeline_registry_(pipeline_registry) {}

    PipelineRegistry& pipeline_registry_;
};

} // namespace renderer
