#pragma once

#include <any>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace renderer {

// TODO: Find a way to separate buffer semantics and dimensionality from the buffer handle

template<typename PixelType>
class FrameBuffer {
public:
    FrameBuffer(std::uint32_t w, std::uint32_t h): width_(w), height_(h) {
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
class BufferHandle {
public:
    BufferHandle(const BufferHandle&) = default;
    BufferHandle& operator=(const BufferHandle&) = default;
    BufferHandle(BufferHandle&&) = default;
    BufferHandle& operator=(BufferHandle&&) = default;

    bool operator==(const BufferHandle& other) const {
        return id_ == other.id_;
    }

private:
    friend class BufferRegistry;
    BufferHandle(int id): id_(id) {}
    int id_;
};

class BufferRegistry {
public:
    template<typename PixelType>
    BufferHandle<PixelType> create_buffer(std::uint32_t width, std::uint32_t height);

    template<typename PixelType>
    FrameBuffer<PixelType>* get_buffer(BufferHandle<PixelType> handle);

private:
    std::unordered_map<std::uint32_t, std::any> buffers_;
    int next_handle_ = 1;
};

template<typename PixelType>
BufferHandle<PixelType> BufferRegistry::create_buffer(std::uint32_t width, std::uint32_t height) {
    std::uint32_t id = next_handle_++;
    buffers_.emplace(id, FrameBuffer<PixelType>(width, height));
    return BufferHandle<PixelType>(id);
}

template<typename PixelType>
FrameBuffer<PixelType>* BufferRegistry::get_buffer(BufferHandle<PixelType> handle) {
    auto it = buffers_.find(handle.id_);
    if (it == buffers_.end()) {
        return nullptr;
    }
    return std::any_cast<FrameBuffer<PixelType>>(&it->second);
}

} // namespace renderer
