#include "web/presenter.hpp"
#include <cstdint>

namespace web {

extern "C" {
void render_to_dom(const void* buffer_ptr, std::uint32_t width, std::uint32_t height);
}

void BrowserPresenter::init() {
    EM_ASM({
        if (!Module.frameTarget) {
            frameTarget = document.getElementById("pre");
            frameTarget.id = "frame-target";
            frameTarget.style.fontFamily = "monospace";
            frameTarget.style.backgroundColor = '#000';
            frameTarget.style.color = "#fff";
            frameTarget.style.padding = "10px";
            Module.frameTarget = frameTarget;
            document.body.appendChild(Module.frameTarget);
        }
    });
}

void BrowserPresenter::deinit() {}

void BrowserPresenter::present(
    const renderer::FrameBufferPrev<renderer::CharacterPixel>& front_buffer,
    const renderer::FrameBufferPrev<renderer::CharacterPixel>&
) {
    render_to_dom(&front_buffer.data(), front_buffer.width(), front_buffer.height());
}

} // namespace web
