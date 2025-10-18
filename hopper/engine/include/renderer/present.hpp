#pragma once

#include "renderer/buffer.hpp"
#include "renderer/types.hpp"

namespace renderer {

class Presenter {
public:
    virtual ~Presenter() = default;
    virtual void init() = 0;
    virtual void deinit() = 0;
    virtual void present(
        const FrameBufferPrev<CharacterPixel>& front_buffer,
        const FrameBufferPrev<CharacterPixel>& back_buffer
    ) = 0;
};

} // namespace renderer
