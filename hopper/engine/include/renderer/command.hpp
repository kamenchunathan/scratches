#pragma once

#include <string>

#include "renderer/encoder.hpp"

namespace renderer {

class RenderCommand {
public:
    virtual const std::string target_pass() const = 0;
    virtual void execute(RenderPassEncoderPrev&) = 0;
    virtual ~RenderCommand() = default;
};

} // namespace renderer
