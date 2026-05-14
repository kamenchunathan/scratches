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
        const std::span<const renderer::CharacterPixel> front_buffer,
        const std::span<const renderer::CharacterPixel> back_buffer,
        std::size_t width,
        std::size_t height
    ) override;
};

} // namespace web

#endif
