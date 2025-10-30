#include "common.hpp"

#ifdef PLATFORM_WASM

    #include <cstdint>
    #include <emscripten.h>
    #include <emscripten/html5.h>

    #include "web/presenter.hpp"

namespace web {

extern "C" {
void render_to_dom(const void* buffer_ptr, std::uint32_t width, std::uint32_t height);
}

void BrowserPresenter::init() {
    // Initialize the web rendering context
    EM_ASM({
        if (!Module.frameContainer) {
            Module.frameContainer = document.getElementById('frame-container');
        }

        if (!Module.frameContainer) {
            Module.frameContainer = document.createElement('pre');
            Module.frameContainer.id = 'frame-container';
            Module.frameContainer.style.fontFamily = 'monospace';
            Module.frameContainer.style.whiteSpace = 'pre';
            Module.frameContainer.style.backgroundColor = '#000';
            Module.frameContainer.style.color = '#fff';
            Module.frameContainer.style.padding = '10px';
            document.body.appendChild(Module.frameContainer);
        }
    });
}

void BrowserPresenter::deinit() {}

void BrowserPresenter::present(
    const renderer::FrameBufferPrev<renderer::CharacterPixel>& front_buffer,
    const renderer::FrameBufferPrev<renderer::CharacterPixel>&
) {
    render_to_dom(front_buffer.data().data(), front_buffer.width(), front_buffer.height());
}

} // namespace web

#endif
