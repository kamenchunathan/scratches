#pragma once

#include <memory>

#include "renderer/types.hpp"

namespace renderer {

class Presenter {
public:
    virtual ~Presenter() = default;
    virtual void present(const OutputBuffers& front_buffer, const OutputBuffers& back_buffer) = 0;
};

} // namespace renderer
