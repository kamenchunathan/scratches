#pragma once

#include <string>

#include "renderer/encoder.hpp"

#include "version.hpp"

namespace renderer {

class RenderCommand {
public:
    virtual const std::string target_pass() const = 0;
    virtual void execute(RenderPassEncoder&) = 0;
    virtual ~RenderCommand() = default;
};

class HOPPER_DEPRECATED(0, 2, "renderer::RenderCommand") RenderCommandPrev {
public:
    virtual const std::string target_pass() const = 0;
    virtual void execute(RenderPassEncoderPrev&) = 0;
    virtual ~RenderCommandPrev() = default;
};

} // namespace renderer
