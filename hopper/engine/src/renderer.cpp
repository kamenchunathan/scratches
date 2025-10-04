#include "renderer.hpp"
#include "renderer/encoder.hpp"

#include <cassert>
#include <memory>
#include <ostream>
#include <print>

namespace renderer {

Renderer::Renderer(std::uint32_t w, std::uint32_t h, std::unique_ptr<Presenter> presenter):
    viewport_width_(w),
    viewport_height_(h),
    presenter_(std::move(presenter)),
    front_buffer_(buffer_registry.create_buffer<CharacterPixel>(w, h)),
    back_buffer_(buffer_registry.create_buffer<CharacterPixel>(w, h)),
    mask_buffer_(buffer_registry.create_buffer<bool>(w, h)) {
    assert(h % 2 == 0 && "The canvas height must be a multiple of 2");
}

BufferHandle<CharacterPixel> Renderer::render_target_handle() const {
    return front_buffer_;
}

BufferHandle<bool> Renderer::mask_buffer_handle() const {
    return mask_buffer_;
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
    RenderPassEncoder encoder(pipeline_registry, buffer_registry, resource_registry);
    const std::vector<RenderPass*>& passes = render_graph.compile();
    for (auto pass: passes) {
        auto it = command_queues_.find(pass->name());
        if (it == command_queues_.end()) {
            std::println("Command submitted with a target renderpass that is not set up");
        }
        std::vector<std::unique_ptr<renderer::RenderCommand>>& commands = it->second;
        for (std::size_t i = 0; i < commands.size(); ++i) {
            commands[i]->execute(encoder);
        }
    }
    present();
    swap_buffers();
}

} // namespace renderer
