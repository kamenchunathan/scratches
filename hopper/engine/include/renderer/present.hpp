#pragma once

#include <memory>

#include "renderer/types.hpp"

namespace renderer {

class Presenter {
public:
    virtual ~Presenter() = default;
    virtual void present(const FrameBuffer& front_buffer, const FrameBuffer& back_buffer) = 0;
};

} // namespace renderer
