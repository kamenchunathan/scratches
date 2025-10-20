#pragma once

#include <any>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace renderer {
template<typename PixelType>
class FrameBufferPrev {
public:
    FrameBufferPrev(std::uint32_t w, std::uint32_t h): width_(w), height_(h) {
        buf_.resize(width_ * height_);
    };

    void update_buffer(std::vector<PixelType> new_buffer) {
        // TODO: Add assertion for size
        buf_ = std::move(new_buffer);
    }

    const std::vector<PixelType>& data() const {
        return buf_;
    }

    std::vector<PixelType>& data() {
        return buf_;
    }

    std::uint32_t width() const {
        return width_;
    }

    std::uint32_t height() const {
        return height_;
    }

private:
    std::vector<PixelType> buf_;
    std::uint32_t width_, height_;
};

class BufferRegistry;

template<typename PixelType>
class BufferHandlePrev {
public:
    BufferHandlePrev(const BufferHandlePrev&) = default;
    BufferHandlePrev& operator=(const BufferHandlePrev&) = default;
    BufferHandlePrev(BufferHandlePrev&&) = default;
    BufferHandlePrev& operator=(BufferHandlePrev&&) = default;

    bool operator==(const BufferHandlePrev& other) const {
        return id_ == other.id_;
    }

private:
    friend class BufferRegistry;
    BufferHandlePrev(int id): id_(id) {}
    int id_;
};

class BufferRegistry {
public:
    template<typename PixelType>
    BufferHandlePrev<PixelType> create_buffer(std::uint32_t width, std::uint32_t height);

    template<typename PixelType>
    FrameBufferPrev<PixelType>* get_buffer(BufferHandlePrev<PixelType> handle);

private:
    std::unordered_map<std::uint32_t, std::any> buffers_;
    int next_handle_ = 1;
};

template<typename PixelType>
BufferHandlePrev<PixelType>
BufferRegistry::create_buffer(std::uint32_t width, std::uint32_t height) {
    std::uint32_t id = next_handle_++;
    buffers_.emplace(id, FrameBufferPrev<PixelType>(width, height));
    return BufferHandlePrev<PixelType>(id);
}

template<typename PixelType>
FrameBufferPrev<PixelType>* BufferRegistry::get_buffer(BufferHandlePrev<PixelType> handle) {
    auto it = buffers_.find(handle.id_);
    if (it == buffers_.end()) {
        return nullptr;
    }
    return std::any_cast<FrameBufferPrev<PixelType>>(&it->second);
}

} // namespace renderer
