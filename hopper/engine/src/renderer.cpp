#include <cassert>
#include <memory>
#include <print>
#include <tuple>

#include "renderer.hpp"
#include "renderer/buffer.hpp"
#include "renderer/encoder.hpp"
#include "renderer/resource.hpp"

namespace renderer {

Renderer::Renderer(std::uint32_t w, std::uint32_t h, std::unique_ptr<Presenter> presenter):
    viewport_width(w),
    viewport_height(h),
    presenter_(std::move(presenter)) {
    TextureHandle<CharacterPixel> char_texture_handle =
        resource_registry.add_texture<CharacterPixel>(w, h).value();

    TextureHandle<CharacterPixel> back_char_texture_handle =
        resource_registry.add_texture<CharacterPixel>(w, h).value();

    front_buffer_ = FrameBuffer {
        .attachments = std::make_tuple(Attachment {
            .view =
                TextureView {
                    .texture = char_texture_handle,
                    .x = 0,
                    .y = 0,
                    .widht = w,
                    .height = h
                },
            .load = LoadOp::Load,
            .store = StoreOp::Store,
        }

        )
    };

    back_buffer_ = FrameBuffer {
        .attachments = std::make_tuple(Attachment {
            .view =
                TextureView {
                    .texture = back_char_texture_handle,
                    .x = 0,
                    .y = 0,
                    .widht = w,
                    .height = h
                },
            .load = LoadOp::Load,
            .store = StoreOp::Store,
        }

        )
    };
    presenter_->init();
}

Renderer::~Renderer() {
    presenter_->deinit();
}

FrameBuffer<Attachment<CharacterPixel>> Renderer::render_target() const {
    return front_buffer_.value();
}

void Renderer::submit(std::unique_ptr<RenderCommand> command) {
    command_queues_[command->target_pass()].push_back(std::move(command));
}

void Renderer::present() {
    const Texture2D<renderer::CharacterPixel>* fb =
        resource_registry.get_texture(std::get<0>(front_buffer_.value().attachments).view.texture)
            .value();
    FrameBufferPrev<renderer::CharacterPixel> front_buf_prev(fb->width(), fb->height());
    front_buf_prev.update_buffer(fb->data());

    const Texture2D<renderer::CharacterPixel>* bb =
        resource_registry.get_texture(std::get<0>(front_buffer_.value().attachments).view.texture)
            .value();
    FrameBufferPrev<renderer::CharacterPixel> back_buf_prev(bb->width(), bb->height());
    back_buf_prev.update_buffer(bb->data());

    presenter_->present(front_buf_prev, back_buf_prev);
}

void Renderer::swap_buffers() {
    // BUG:  New Renderer Implementation does not swap buffers.
    // Shouldn't matter since presenter only ever presents the front buffer anyway
}

void Renderer::render_frame() {
    RenderPassEncoder encoder(resource_registry);
    const std::vector<RenderPass*>& passes = render_graph.compile();
    for (auto pass: passes) {
        auto it = command_queues_.find(pass->name());
        if (it == command_queues_.end()) {
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

BufferHandlePrev<CharacterPixel> RendererPrev::render_target_handle() const {
    return front_buffer_handle_;
}

BufferHandlePrev<bool> RendererPrev::mask_buffer_handle() const {
    return mask_buffer_;
}

void RendererPrev::submit(std::unique_ptr<RenderCommandPrev> command) {
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
    RenderPassEncoderPrev encoder(pipeline_registry, buffer_registry, shader_resource_registry);
    const std::vector<RenderPass*>& passes = render_graph.compile();
    for (auto pass: passes) {
        auto it = command_queues_.find(pass->name());
        if (it == command_queues_.end()) {
            std::println("Command submitted with a target renderpass that is not set up");
            continue;
        }
        std::vector<std::unique_ptr<renderer::RenderCommandPrev>>& commands = it->second;
        for (std::size_t i = 0; i < commands.size(); ++i) {
            commands[i]->execute(encoder);
        }
    }
    command_queues_.clear();
    present();
    swap_buffers();
}

} // namespace renderer
