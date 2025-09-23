#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

#include "renderer/present.hpp"
#include "renderer/present/term.hpp"
#include "renderer/types.hpp"

namespace renderer {

class Renderer {
public:
    FrameBuffer front_buffer, back_buffer;

    Renderer(std::uint32_t canvas_width, std::uint32_t canvas_height):
        front_buffer(canvas_height, canvas_width),
        back_buffer(canvas_height, canvas_width),
        presenter_(std::make_unique<TerminalPresenter>()) {
        assert(canvas_height % 2 == 0 && "The canvas height must be a multiple of 2");
    };

    void present();

private:
    std::unique_ptr<Presenter> presenter_;
};

} // namespace renderer
