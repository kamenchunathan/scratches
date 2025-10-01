#include "renderer.hpp"

#include <cassert>
#include <memory>

namespace renderer {

Renderer::Renderer(std::uint32_t w, std::uint32_t h):
    viewport_width_(w),
    viewport_height_(h),
    front_buffer_(buffer_registry.create_buffer<CharacterPixel>(w, h)),
    back_buffer_(buffer_registry.create_buffer<CharacterPixel>(w, h)),
    mask_buffer_(buffer_registry.create_buffer<bool>(w, h)) {
    assert(h % 2 == 0 && "The canvas height must be a multiple of 2");
}

void Renderer::submit(std::unique_ptr<RenderCommand> command) {
    command_queues_[command->target_pass()].push_back(std::move(command));
}

void Renderer::present() {
    auto* front = buffer_registry.get_buffer(front_buffer_);
    auto* back = buffer_registry.get_buffer(back_buffer_);
    presenter_->present(*front, *back);
}

void Renderer::swap_buffers() {
    std::swap(front_buffer_, back_buffer_);
}

void Renderer::render_frame() {
    std::printf("Hello world\n");
}

} // namespace renderer
