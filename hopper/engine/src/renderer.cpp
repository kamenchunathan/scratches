#include "renderer.hpp"
#include "renderer/encoder.hpp"

#include <cassert>
#include <memory>
#include <print>

namespace renderer {

RendererPrev::RendererPrev(std::uint32_t w, std::uint32_t h, std::unique_ptr<Presenter> presenter):
    viewport_width_(w),
    viewport_height_(h),
    presenter_(std::move(presenter)),
    front_buffer_handle_(buffer_registry.create_buffer<CharacterPixel>(w, h)),
    back_buffer_handle_(buffer_registry.create_buffer<CharacterPixel>(w, h)),
    mask_buffer_(buffer_registry.create_buffer<bool>(w, h)) {
    presenter_->init();
}

RendererPrev::~RendererPrev() {
    presenter_->deinit();
}

BufferHandle<CharacterPixel> RendererPrev::render_target_handle() const {
    return front_buffer_handle_;
}

BufferHandle<bool> RendererPrev::mask_buffer_handle() const {
    return mask_buffer_;
}

void RendererPrev::submit(std::unique_ptr<RenderCommand> command) {
    command_queues_[command->target_pass()].push_back(std::move(command));
}

void RendererPrev::present() {
    auto* front = buffer_registry.get_buffer(front_buffer_handle_);
    auto* back = buffer_registry.get_buffer(back_buffer_handle_);
    presenter_->present(*front, *back);
}

void RendererPrev::swap_buffers() {
    auto* front = buffer_registry.get_buffer(front_buffer_handle_);
    auto* back = buffer_registry.get_buffer(back_buffer_handle_);

    std::swap(front->data(), back->data());
}

void RendererPrev::render_frame() {
    RenderPassEncoderPrev encoder(pipeline_registry, buffer_registry, resource_registry);
    const std::vector<RenderPass*>& passes = render_graph.compile();
    for (auto pass: passes) {
        auto it = command_queues_.find(pass->name());
        if (it == command_queues_.end()) {
            std::println("Command submitted with a target renderpass that is not set up");
            continue;
        }
        std::vector<std::unique_ptr<renderer::RenderCommand>>& commands = it->second;
        for (std::size_t i = 0; i < commands.size(); ++i) {
            commands[i]->execute(encoder);
        }
    }
    command_queues_.clear();
    present();
    swap_buffers();
}

} // namespace renderer
