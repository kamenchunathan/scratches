#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>

#include "renderer/buffer.hpp"
#include "renderer/present.hpp"
#include "renderer/shader.hpp"

namespace renderer {

class Renderer {
public:
    Renderer(std::uint32_t w, std::uint32_t h);

    void render_frame();

    template<typename Pipeline>
    void register_pipeline(std::unique_ptr<Pipeline> pipeline);

private:
    std::uint32_t viewport_width_, viewport_height_;
    std::unique_ptr<Presenter> presenter_ = nullptr;

    BufferRegistry buffer_registry_;
    BufferHandle<CharacterPixel> front_buffer_;
    BufferHandle<CharacterPixel> back_buffer_;
    BufferHandle<bool> mask_buffer_;

    std::unordered_map<std::type_index, std::unique_ptr<IPipeline>> pipelines_;

    void present();
    void swap_buffers();
};

template<typename Pipeline>
void Renderer::register_pipeline(std::unique_ptr<Pipeline> pipeline) {
    pipelines_.insert_or_assign(std::type_index(typeid(Pipeline)), std::move(pipeline));
}

} // namespace renderer
