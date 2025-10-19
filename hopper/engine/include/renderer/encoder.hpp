#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include <Eigen/Dense>

#include "renderer/buffer.hpp"
#include "renderer/resource.hpp"
#include "renderer/shader.hpp"

namespace renderer {

template<typename Pipeline, typename... Attachments>
    requires FragOutMatchesAttachments<typename Pipeline::frag_out, Attachments...>
struct DrawDescriptor {
    FrameBuffer<Attachments...> target;
    BufferHandlePrev<typename Pipeline::vertex_in> vertices;
    std::uint32_t vertex_count;
    std::uint32_t first_vertex = 0;
};

class RenderPassEncoderPrev {
public:
    template<typename VertexIn, typename VertexOut, typename FragOut, typename... RequiredResources>
    void draw_prev(
        BufferHandlePrev<VertexIn> vertex_buf_handle,
        BufferHandlePrev<FragOut> output_buffer_handle,
        std::uint32_t vertex_count,
        std::uint32_t first_vertex = 0
    );

    template<typename Pipeline, typename... Attachments>
        requires FragOutMatchesAttachments<typename Pipeline::frag_out, Attachments...>
    void draw(
        FrameBuffer<Attachments...> target,
        BufferHandlePrev<typename Pipeline::vertex_in> vertices,
        std::uint32_t vertex_count,
        std::uint32_t first_vertex = 0
    );

    void draw_indexed(
        const std::vector<uint32_t>& indices,
        uint32_t first_index = 0,
        uint32_t count = 0
    );

private:
    friend class RendererPrev;

    RenderPassEncoderPrev(
        PipelineRegistry& pipeline_registry,
        BufferRegistry& buffer_registry,
        ShaderResourceRegistry& resource_registry
    ):
        pipeline_registry_(pipeline_registry),
        buffer_registry_(buffer_registry),
        resource_registry_(resource_registry) {}

    PipelineRegistry& pipeline_registry_;
    BufferRegistry& buffer_registry_;
    ShaderResourceRegistry& resource_registry_;
};

// A rasterizer based on scratchapixel lesson
// https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage.html
// and the paper cited in this lesson https://www.cs.drexel.edu/~deb39/Classes/Papers/comp175-06-pineda.pdf
inline float
edge_function(const Eigen::Vector2f& a, const Eigen::Vector2f& b, const Eigen::Vector2f& c) {
    return (c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]);
}

template<typename VertexIn, typename VertexOut, typename FragOut, typename... RequiredResources>
void RenderPassEncoderPrev::draw_prev(
    BufferHandlePrev<VertexIn> vertex_buffer_handle,
    BufferHandlePrev<FragOut> output_buffer_handle,
    std::uint32_t vertex_count,
    std::uint32_t first_vertex
) {
    // TODO: Better error handling
    PipelinePrev<VertexIn, VertexOut, FragOut, RequiredResources...>* pipeline =
        pipeline_registry_
            .get_pipeline<PipelinePrev<VertexIn, VertexOut, FragOut, RequiredResources...>>();

    if (!pipeline) {
        return;
    }

    FrameBufferPrev<VertexIn>* vb = buffer_registry_.get_buffer(vertex_buffer_handle);
    FrameBufferPrev<FragOut>* out_buffer = buffer_registry_.get_buffer(output_buffer_handle);
    auto resources = resource_registry_.extract<RequiredResources...>();

    std::vector<VertexIn> vertex_data = vb->data();
    if (first_vertex >= vertex_data.size()) {
        return;
    }
    std::uint32_t end_vertex =
        std::min(first_vertex + vertex_count, static_cast<std::uint32_t>(vertex_data.size()));
    std::uint32_t actual_vertex_count = end_vertex - first_vertex;

    assert(actual_vertex_count % 3 == 0 && "Vertices must be a multiple of 3");
    std::vector<VertexOut> v_out;
    v_out.reserve(actual_vertex_count);

    for (std::size_t i = 0; i < actual_vertex_count; ++i) {
        v_out.push_back(std::apply(
            [&](auto&&... args) {
                return pipeline->shader->vertex(vertex_data[first_vertex + i], args...);
            },
            resources
        ));
    }

    const std::uint32_t imageWidth = out_buffer->width();
    const std::uint32_t imageHeight = out_buffer->height();

    std::vector<FragOut>& new_frame_buffer_data = out_buffer->data();
    std::vector<float> z_buffer(imageWidth * imageHeight, std::numeric_limits<float>::infinity());

    for (std::size_t i = 0; i < v_out.size(); i += 3) {
        const VertexOut& v0_clip = v_out[i];
        const VertexOut& v1_clip = v_out[i + 1];
        const VertexOut& v2_clip = v_out[i + 2];

        // Perspective division
        Eigen::Vector3f v0_ndc = v0_clip.position.template head<3>() / v0_clip.position.w();
        Eigen::Vector3f v1_ndc = v1_clip.position.template head<3>() / v1_clip.position.w();
        Eigen::Vector3f v2_ndc = v2_clip.position.template head<3>() / v2_clip.position.w();

        // Viewport transform
        Eigen::Vector2f v0_screen = {
            (v0_ndc.x() + 1.0f) * 0.5f * imageWidth,
            (1.0f - (v0_ndc.y() + 1.0f) * 0.5f) * imageHeight
        };
        Eigen::Vector2f v1_screen = {
            (v1_ndc.x() + 1.0f) * 0.5f * imageWidth,
            (1.0f - (v1_ndc.y() + 1.0f) * 0.5f) * imageHeight
        };
        Eigen::Vector2f v2_screen = {
            (v2_ndc.x() + 1.0f) * 0.5f * imageWidth,
            (1.0f - (v2_ndc.y() + 1.0f) * 0.5f) * imageHeight
        };

        // Bounding box of the triangle
        int xmin = std::max(
            0,
            static_cast<int>(std::floor(std::min({v0_screen.x(), v1_screen.x(), v2_screen.x()})))
        );
        int ymin = std::max(
            0,
            static_cast<int>(std::floor(std::min({v0_screen.y(), v1_screen.y(), v2_screen.y()})))
        );
        int xmax = std::min(
            static_cast<int>(imageWidth - 1),
            static_cast<int>(std::ceil(std::max({v0_screen.x(), v1_screen.x(), v2_screen.x()})))
        );
        int ymax = std::min(
            static_cast<int>(imageHeight - 1),
            static_cast<int>(std::ceil(std::max({v0_screen.y(), v1_screen.y(), v2_screen.y()})))
        );

        float area = edge_function(v0_screen, v1_screen, v2_screen);

        for (int y = ymin; y <= ymax; ++y) {
            for (int x = xmin; x <= xmax; ++x) {
                Eigen::Vector2f p = {static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};

                float w0 = edge_function(v1_screen, v2_screen, p);
                float w1 = edge_function(v2_screen, v0_screen, p);
                float w2 = edge_function(v0_screen, v1_screen, p);

                Eigen::Vector2f edge0 = v2_screen - v1_screen;
                Eigen::Vector2f edge1 = v0_screen - v2_screen;
                Eigen::Vector2f edge2 = v1_screen - v0_screen;

                bool overlaps = true;
                overlaps &=
                    (w0 == 0 ? ((edge0.y() == 0 && edge0.x() > 0) || edge0.y() > 0) : (w0 > 0));
                overlaps &=
                    (w1 == 0 ? ((edge1.y() == 0 && edge1.x() > 0) || edge1.y() > 0) : (w1 > 0));
                overlaps &=
                    (w2 == 0 ? ((edge2.y() == 0 && edge2.x() > 0) || edge2.y() > 0) : (w2 > 0));

                if (overlaps) {
                    float bc0 = w0 / area;
                    float bc1 = w1 / area;
                    float bc2 = w2 / area;

                    float z_interpolated = bc0 * v0_ndc.z() + bc1 * v1_ndc.z() + bc2 * v2_ndc.z();

                    if (z_interpolated < z_buffer[y * imageWidth + x]) {
                        float one_over_w0 = 1.0f / v0_clip.position.w();
                        float one_over_w1 = 1.0f / v1_clip.position.w();
                        float one_over_w2 = 1.0f / v2_clip.position.w();

                        float w_interp_reciprocal =
                            1.0f / (bc0 * one_over_w0 + bc1 * one_over_w1 + bc2 * one_over_w2);

                        auto bc0_persp = bc0 * one_over_w0 * w_interp_reciprocal;
                        auto bc1_persp = bc1 * one_over_w1 * w_interp_reciprocal;
                        auto bc2_persp = bc2 * one_over_w2 * w_interp_reciprocal;

                        auto t0 = detail::tuple_from_aggregate(v0_clip);
                        auto t1 = detail::tuple_from_aggregate(v1_clip);
                        auto t2 = detail::tuple_from_aggregate(v2_clip);

                        constexpr auto size = std::tuple_size_v<decltype(t0)>;

                        auto interpolated_tuple = std::apply(
                            [&](auto&&... args0) {
                                return std::apply(
                                    [&](auto&&... args1) {
                                        return std::apply(
                                            [&](auto&&... args2) {
                                                return std::make_tuple(
                                                    (args0 * bc0_persp + args1 * bc1_persp
                                                     + args2 * bc2_persp)...
                                                );
                                            },
                                            t2
                                        );
                                    },
                                    t1
                                );
                            },
                            t0
                        );

                        VertexOut interpolated_v =
                            detail::construct_from_tuple<VertexOut>(std::move(interpolated_tuple));

                        z_buffer[y * imageWidth + x] = z_interpolated;
                        new_frame_buffer_data[y * imageWidth + x] = std::apply(
                            [&](auto&&... args) {
                                return pipeline->shader->fragment(interpolated_v, args...);
                            },
                            resources
                        );
                    }
                }
            }
        }
    }
}

} // namespace renderer
