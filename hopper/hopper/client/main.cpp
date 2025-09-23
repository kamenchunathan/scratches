#include "color.hpp"
#include "renderer.hpp"

void draw_filled_rect(
    renderer::FrameBuffer& buffer,
    int x,
    int y,
    int w,
    int h,
    core::ColorRGBA32F color
) {
    for (int i = x; i < x + w; ++i) {
        for (int j = y; j < y + h; ++j) {
            if (i >= 0 && i < buffer.pixel_width() && j >= 0 && j < buffer.pixel_height()) {
                buffer.pixel_buf[j * buffer.pixel_width() + i] = color;
            }
        }
    }
}

int main() {
    renderer::Renderer app_renderer(160, 90);

    app_renderer.front_buffer.clear({ 0.8f, 0.3f, 0.0f, 1.0f });

    draw_filled_rect(app_renderer.front_buffer, 10, 10, 20, 20, { 0.0f, 1.0f, 0.0f, 1.0f });
    draw_filled_rect(app_renderer.front_buffer, 40, 10, 20, 20, { 0.0f, 0.0f, 1.0f, 1.0f });
    draw_filled_rect(app_renderer.front_buffer, 10, 40, 20, 20, { 1.0f, 0.0f, 0.0f, 1.0f });
    draw_filled_rect(app_renderer.front_buffer, 40, 40, 20, 20, { 1.0f, 1.0f, 0.0f, 1.0f });

    app_renderer.present();
}
