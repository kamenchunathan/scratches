#pragma once

#ifdef PLATFORM_WASM

    #include "renderer/present.hpp"

namespace web {

class BrowserPresenter: public renderer::Presenter {
public:
    BrowserPresenter() {};
    void init() override;
    void deinit() override;
    void present(
        const renderer::FrameBufferPrev<renderer::CharacterPixel>& front_buffer,
        const renderer::FrameBufferPrev<renderer::CharacterPixel>& back_buffer
    ) override;
};

} // namespace web

#endif
