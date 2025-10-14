#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>
namespace renderer {

template<typename PixelType>
class Texture2D {
public:
    Texture2D(): width_(0), height_(0) {}
    Texture2D(std::uint32_t width, std::uint32_t height): width_(width), height_(height) {
        texture_data_.resize(width * height);
    }
    Texture2D(std::uint32_t width, std::uint32_t height, std::vector<PixelType> data):
        width_(width),
        height_(height),
        texture_data_(std::move(data)) {}

    [[nodiscard]] std::uint32_t width() const {
        return width_;
    }

    [[nodiscard]] std::uint32_t height() const {
        return height_;
    }

    [[nodiscard]] const std::vector<PixelType>& data() const {
        return texture_data_;
    }

private:
    std::uint32_t width_, height_;
    std::vector<PixelType> texture_data_;
};

enum class FilterMode { Nearest, Linear };
enum class WrapMode { Repeat, Clamp };

template<typename PixelType>
class Sampler {
public:
    PixelType sample(
        FilterMode filter_mode,
        WrapMode wrap,
        const Texture2D<PixelType>& tex,
        float u,
        float v
    ) {
        std::int32_t x, y;

        if (filter_mode == FilterMode::Nearest) {
            x = static_cast<std::int32_t>(u * tex.width());
            y = static_cast<std::int32_t>(v * tex.width());

        } else {
            assert(false && "Unimplemented");
        }

        std::int32_t wrapped_x, wrapped_y;
        switch (wrap) {
            case WrapMode::Repeat:
                wrapped_x = x % static_cast<std::int32_t>(tex.width());
                wrapped_x = wrapped_x > 0 ? wrapped_x : wrapped_x + tex.width();
                wrapped_y = y % static_cast<std::int32_t>(tex.height());
                wrapped_y = wrapped_y > 0 ? wrapped_y : wrapped_y + tex.height();
                break;
            case WrapMode::Clamp:
                wrapped_x = std::clamp(x, 0, tex.width());
                wrapped_y = std::clamp(y, 0, tex.height());
                break;
        }

        return tex.data()[wrapped_x * tex.width() + wrapped_y];
    }
};

} // namespace renderer
