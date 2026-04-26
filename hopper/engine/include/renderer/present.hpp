#pragma once

#include "renderer/types.hpp"
#include <cstddef>

namespace renderer {

class Presenter {
public:
    virtual ~Presenter() = default;
    virtual void init() = 0;
    virtual void deinit() = 0;
    virtual void present(
        const std::span<const renderer::CharacterPixel> front_buffer,
        const std::span<const renderer::CharacterPixel> back_buffer,
        std::size_t width,
        std::size_t height
    ) = 0;
};

} // namespace renderer
