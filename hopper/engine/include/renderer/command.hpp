#pragma once

#include <string>

#include "renderer/encoder.hpp"

namespace renderer {

class RenderCommand {
public:
    virtual const std::string target_pass() const = 0;
    virtual void execute(RenderPassEncoder&) = 0;
    virtual ~RenderCommand() = default;
};

class RenderCommandPrev {
public:
    virtual const std::string target_pass() const = 0;
    virtual void execute(RenderPassEncoderPrev&) = 0;
    virtual ~RenderCommandPrev() = default;
};

} // namespace renderer
