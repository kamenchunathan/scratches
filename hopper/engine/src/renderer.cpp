#include <algorithm>
#include <iostream>
#include <print>
#include <ranges>

#include "renderer.hpp"
#include "renderer/types.hpp"

namespace renderer {

void Renderer::present() {
    presenter_->present(front_buffer, back_buffer);
}

} // namespace renderer
