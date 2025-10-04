#pragma once

#include <cstdint>
#include <vector>

#include "renderer/buffer.hpp"
#include "renderer/resource.hpp"
#include "renderer/shader.hpp"

namespace renderer {

class RenderPassEncoder {
public:
    template<typename VertexIn, typename VertexOut, typename FragOut, typename... RequiredResources>
    void draw(
        BufferHandle<VertexIn> vertex_buf_handle,
        BufferHandle<FragOut> output_buffer_handle,
        std::uint32_t vertex_count,
        std::uint32_t first_vertex = 0
    );

    void draw_indexed(
        const std::vector<uint32_t>& indices,
        uint32_t first_index = 0,
        uint32_t count = 0
    );

private:
    friend class Renderer;

    RenderPassEncoder(
        PipelineRegistry& pipeline_registry,
        BufferRegistry& buffer_registry,
        ResourceRegistry& resource_registry
    ):
        pipeline_registry_(pipeline_registry),
        buffer_registry_(buffer_registry),
        resource_registry_(resource_registry) {}

    PipelineRegistry& pipeline_registry_;
    BufferRegistry& buffer_registry_;
    ResourceRegistry& resource_registry_;
};

template<typename VertexIn, typename VertexOut, typename FragOut, typename... RequiredResources>
void RenderPassEncoder::draw(
    BufferHandle<VertexIn> vertex_buffer_handle,
    BufferHandle<FragOut> output_buffer_handle,
    std::uint32_t vertex_count,
    std::uint32_t first_vertex
) {
    // TODO: Better error handling
    Pipeline<VertexIn, VertexOut, FragOut, RequiredResources...>* pipeline =
        pipeline_registry_
            .get_pipeline<Pipeline<VertexIn, VertexOut, FragOut, RequiredResources...>>();

    if (!pipeline) {
        return;
    }

    FrameBuffer<VertexIn>* vb = buffer_registry_.get_buffer(vertex_buffer_handle);
    FrameBuffer<FragOut>* out_buffer = buffer_registry_.get_buffer(output_buffer_handle);
    ResourcePack<RequiredResources...> resources =
        resource_registry_.extract<RequiredResources...>();

    std::vector<VertexIn> vertex_data = vb->data();
    if (first_vertex >= vertex_data.size()) {
        return;
    }
    std::uint32_t end_vertex =
        std::min(first_vertex + vertex_count, static_cast<std::uint32_t>(vertex_data.size()));
    std::uint32_t actual_vertex_count = end_vertex - first_vertex;

    std::vector<VertexOut> v_out;
    v_out.reserve(actual_vertex_count);

    for (std::size_t i = 0; i < actual_vertex_count; ++i) {
        v_out.push_back(pipeline->shader->vertex(vertex_data[first_vertex + i], resources));
    }

    // TODO: Add primitives  and interpolation
    std::vector<FragOut> fragment_outputs;
    fragment_outputs.reserve(v_out.size());
    for (std::size_t i = 0; i < v_out.size(); ++i) {
        fragment_outputs.push_back(pipeline->shader->fragment(v_out[i], resources));
    }

    out_buffer->update_buffer(fragment_outputs);
}

} // namespace renderer
