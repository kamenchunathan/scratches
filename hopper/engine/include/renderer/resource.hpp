#pragma once

#include <any>
#include <concepts>
#include <expected>
#include <memory>
#include <span>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "renderer/shader.hpp"
#include "renderer/texture.hpp"

namespace renderer {

enum class LoadOp { Load, Clear, Ignore };

enum class StoreOp { Store, Ignroe };

/* Wrapper type to specify tags and allow multiple texture2Ds to exist in resources
 * TODO: Return type in binding directly rather than the binding struct in the shader
 */
template<typename, typename Tag>
struct ResourceHandle {
public:
    template<typename T>
    bool operator==(const ResourceHandle<T, Tag>& other) const noexcept {
        return generation_ == other.generation_ && index_ == other.index_;
    }

    template<typename T>
    bool operator!=(const ResourceHandle<T, Tag>& other) const {
        return !(*this == other);
    }

private:
    friend class ResourceRegistry;

    ResourceHandle(std::uint32_t index, std::uint32_t generation):
        index_(index),
        generation_(generation) {};

    std::uint32_t index_;
    std::uint32_t generation_;
};

struct TextureTag {};
struct BufferTag {};
struct FrameBufferTag {};
struct PipelineTag {};

template<typename Format>
struct TextureView {
    ResourceHandle<Format, TextureTag> texture;
    std::uint32_t x, y, widht, height;
};

template<typename Format>
struct Attachment {
    TextureView<Format> view;
    LoadOp load;
    StoreOp store;
};

template<typename... Attachments>
struct FrameBuffer {
    std::tuple<Attachments...> attachments;
};

template<typename Format>
using TextureHandle = ResourceHandle<Format, TextureTag>;

template<typename Element>
using BufferHandle = ResourceHandle<Element, BufferTag>;

template<typename... Attachments>
using FrameBufferHandle = ResourceHandle<FrameBuffer<Attachments...>, FrameBufferTag>;

template<typename Pipeline>
using PipelineHandle = ResourceHandle<Pipeline, PipelineTag>;

enum class ResourceError { InvalidDimensions, InvalidHandle, InvalidPipelineInstance };

class ResourceRegistry {
public:
    /******************************** Texture Operations ****************************************/
    template<typename Format>
    [[nodiscard]] std::expected<TextureHandle<Format>, ResourceError>
    add_texture(std::uint32_t width, std::uint32_t height) {
        return add_texture(Texture2D<Format>(width, height));
    }

    template<typename Format>
    [[nodiscard]] std::expected<TextureHandle<Format>, ResourceError>
    add_texture(Texture2D<Format>&& texture) {
        if (texture.width() == 0 || texture.height() == 0) {
            return std::unexpected(ResourceError::InvalidDimensions);
        }
        return insert_texture(std::move(texture));
    }

    template<typename Format>
    [[nodiscard]] std::expected<TextureHandle<Format>, ResourceError>
    add_texture(std::uint32_t width, std::uint32_t height, std::vector<Format>&& data) {
        if (width == 0 || height == 0 || data.size() != width * height) {
            return std::unexpected(ResourceError::InvalidDimensions);
        }
        return insert_texture(Texture2D<Format>(width, height, std::move(data)));
    }

    template<typename Format>
    [[nodiscard]] std::expected<const Texture2D<Format>*, ResourceError>
    get_texture(TextureHandle<Format> handle) const {
        if (!is_valid(handle)) {
            return std::unexpected(ResourceError::InvalidHandle);
        }
        return get_texture_ptr(handle);
    }

    template<typename Format>
    [[nodiscard]] std::expected<Texture2D<Format>*, ResourceError>
    get_texture_mut(TextureHandle<Format> handle) {
        if (!is_valid(handle)) {
            return std::unexpected(ResourceError::InvalidHandle);
        }
        return get_texture_ptr_mut(handle);
    }

    template<typename Format>
    void destroy_texture(TextureHandle<Format> handle);

    /******************************** Buffer Operations ****************************************/
    template<typename Element>
    [[nodiscard]] std::expected<BufferHandle<Element>, ResourceError>
    add_buffer(std::uint32_t element_count) {
        if (element_count == 0) {
            return std::unexpected(ResourceError::InvalidDimensions);
        }
        return insert_buffer(std::vector<Element>(element_count));
    }

    template<typename Element>
    [[nodiscard]] std::expected<BufferHandle<Element>, ResourceError>
    add_buffer(std::vector<Element>&& data) {
        if (data.empty()) {
            return std::unexpected(ResourceError::InvalidDimensions);
        }
        return insert_buffer(std::move(data));
    }

    template<typename Element>
    [[nodiscard]] std::expected<std::span<const Element>, ResourceError>
    get_buffer(BufferHandle<Element> handle) const {
        if (!is_valid(handle)) {
            return std::unexpected(ResourceError::InvalidHandle);
        }
        const auto* buf = get_buffer_ptr(handle);
        return std::span<const Element>(buf->data(), buf->size());
    }

    template<typename Element>
    [[nodiscard]] std::expected<std::span<Element>, ResourceError>
    get_buffer_mut(BufferHandle<Element> handle) {
        if (!is_valid(handle)) {
            return std::unexpected(ResourceError::InvalidHandle);
        }
        auto* buf = get_buffer_ptr_mut(handle);
        return std::span<Element>(buf->data(), buf->size());
    }

    template<typename Element>
    void destroy_buffer(BufferHandle<Element> handle);

    /*********************************** Pipeline Operations ***********************************/
    template<typename PipelineType>
        requires std::derived_from<PipelineType, IPipeline>
    [[nodiscard]] std::expected<PipelineHandle<PipelineType>, ResourceError>
    add_pipeline(std::unique_ptr<PipelineType> pipeline) {
        if (!pipeline) {
            return std::unexpected(ResourceError::InvalidPipelineInstance);
        }
        return insert_pipeline(std::move(pipeline));
    }

    template<typename PipelineType>
    [[nodiscard]] std::expected<const PipelineType*, ResourceError>
    get_pipeline(PipelineHandle<PipelineType> handle) const {
        if (!is_valid(handle)) {
            return std::unexpected(ResourceError::InvalidHandle);
        }
        return get_pipeline_ptr(handle);
    }

    template<typename PipelineType>
    void destroy_pipeline(PipelineHandle<PipelineType> handle);

    template<typename T, typename Tag>
    [[nodiscard]] bool is_valid(ResourceHandle<T, Tag> handle) const {
        const auto* store = get_store_for_tag<T, Tag>();
        if (!store || handle.index_ >= store->slots.size()) {
            return false;
        }
        const auto& slot = store->slots[handle.index_];
        return slot.generation == handle.generation_ && slot.data.has_value();
    }

private:
    template<typename T, typename Tag>
    auto get_store_for_tag() const {
        if constexpr (std::is_same_v<Tag, TextureTag>) {
            return get_texture_store<T>();
        } else if constexpr (std::is_same_v<Tag, BufferTag>) {
            return get_buffer_store<T>();
        } else if constexpr (std::is_same_v<Tag, PipelineTag>) {
            return &pipeline_store_;
        } else {
            return nullptr;
        }
    }

private:
    template<typename T>
    struct Slot {
        std::uint32_t generation;
        std::optional<T> data;
    };

    template<typename ResourceType>
    struct Store {
        std::vector<Slot<ResourceType>> slots;
        std::vector<std::uint32_t> free_list;
        std::uint32_t next_generation = 1;

        std::uint32_t allocate() {
            std::uint32_t index;
            if (!free_list.empty()) {
                index = free_list.back();
                free_list.pop_back();
            } else {
                index = static_cast<std::uint32_t>(slots.size());
                slots.push_back({0, std::nullopt});
            }

            slots[index].generation = next_generation++;
            return index;
        }

        void destroy(std::uint32_t index) {
            if (index < slots.size() && slots[index].data.has_value()) {
                slots[index].data.reset();
                free_list.push_back(index);
            }
        }
    };

    std::unordered_map<std::type_index, std::any> texture_store_;
    std::unordered_map<std::type_index, std::any> buffer_store_;
    Store<std::unique_ptr<IPipeline>> pipeline_store_;

    template<typename Format>
    Store<Texture2D<Format>>* get_texture_store();

    template<typename Format>
    const Store<Texture2D<Format>>* get_texture_store() const;

    template<typename Element>
    Store<std::vector<Element>>* get_buffer_store();

    template<typename Format>
    std::expected<TextureHandle<Format>, ResourceError> insert_texture(Texture2D<Format>&& texture);

    template<typename Format>
    const Texture2D<Format>* get_texture_ptr(TextureHandle<Format> handle) const;

    template<typename Format>
    Texture2D<Format>* get_texture_ptr_mut(TextureHandle<Format> handle);

    template<typename Element>
    std::expected<BufferHandle<Element>, ResourceError> insert_buffer(std::vector<Element>&& data);

    template<typename Element>
    const std::vector<Element>* get_buffer_ptr(BufferHandle<Element> handle) const;

    template<typename Element>

    std::vector<Element>* get_buffer_ptr_mut(BufferHandle<Element> handle);

    template<typename PipelineType>
    const PipelineType* get_pipeline_ptr(PipelineHandle<PipelineType> handle) const;

    template<typename PipelineType>
    std::expected<PipelineHandle<PipelineType>, ResourceError>
    insert_pipeline(std::unique_ptr<PipelineType>&& pipeline);
};

/* Wrapper type to specify tags and allow multiple texture2Ds to exist in resources
 * TODO: Return type in binding directly rather than the binding struct in the shader
 */
template<typename PixelType, typename Tag>
struct Binding {
    Texture2D<PixelType> inner;
};

class ShaderResourceRegistry {
public:
    template<typename T, typename... Args>
    void bind(Args&&... args) {
        resources_[std::type_index(typeid(T))] = std::make_shared<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    void unbind() {
        resources_.erase(std::type_index(typeid(T)));
    }

    template<typename T>
    T* get() {
        auto it = resources_.find(std::type_index(typeid(T)));
        if (it == resources_.end()) {
            return nullptr;
        }
        if (auto* ptr = std::any_cast<std::shared_ptr<T>>(&it->second)) {
            return ptr->get();
        }
        return nullptr;
    }

    template<typename T>
    const T* get() const {
        auto it = resources_.find(std::type_index(typeid(T)));
        if (it == resources_.end()) {
            return nullptr;
        }
        if (const auto* ptr = std::any_cast<std::shared_ptr<T>>(&it->second)) {
            return ptr->get();
        }
        return nullptr;
    }

    template<typename... RequiredResources>
    auto extract() const {
        // NOTE: Get returns a possibly null pointer but we cast it to a reference
        // This looks like a problem.
        // Perhaps replace it with an optional type if not bound
        return std::make_tuple(*get<RequiredResources>()...);
    }

private:
    std::unordered_map<std::type_index, std::any> resources_;
};

template<typename Format>
ResourceRegistry::Store<Texture2D<Format>>* ResourceRegistry::get_texture_store() {
    auto key = std::type_index(typeid(Format));
    auto it = texture_store_.find(key);
    if (it == texture_store_.end()) {
        it = texture_store_.emplace(key, std::make_unique<Store<Texture2D<Format>>>()).first;
    }
    return std::any_cast<std::unique_ptr<Store<Texture2D<Format>>>>(&it->second)->get();
}

template<typename Elem>
ResourceRegistry::Store<std::vector<Elem>>* ResourceRegistry::get_buffer_store() {
    auto key = std::type_index(typeid(Elem));
    auto it = buffer_store_.find(key);
    if (it == buffer_store_.end()) {
        it = buffer_store_.emplace(key, std::make_unique<Store<std::vector<Elem>>>()).first;
    }
    return std::any_cast<std::unique_ptr<Store<std::vector<Elem>>>>(&it->second)->get();
}

template<typename Format>
std::expected<TextureHandle<Format>, ResourceError>
ResourceRegistry::insert_texture(Texture2D<Format>&& texture) {
    Store<Texture2D<Format>>* store = get_texture_store<Format>();
    auto index = store->allocate();
    auto& slot = store->slots[index];
    slot.data = std::move(texture);

    return TextureHandle<Format>(index, slot.generation);
}

// TODO: Check validity
// Typechecks but I'm unsure if we're returning the correct type as it compiles but this
// may be because it's not instantiated
template<typename Format>
const Texture2D<Format>* ResourceRegistry::get_texture_ptr(TextureHandle<Format> handle) const {
    const auto* store = get_texture_store<Format>();
    return &store->slots[handle.index_].data.value();
}

template<typename Format>
Texture2D<Format>* ResourceRegistry::get_texture_ptr_mut(TextureHandle<Format> handle) {
    auto* store = get_texture_store<Format>();
    return &store->slots[handle.index_].data.value();
}

template<typename Element>
std::expected<BufferHandle<Element>, ResourceError>
ResourceRegistry::insert_buffer(std::vector<Element>&& data) {
    auto* store = get_buffer_store<Element>();
    auto index = store->allocate();
    auto& slot = store->slots[index];
    slot.data = std::move(data);

    return BufferHandle<Element>(index, slot.generation);
}

template<typename Element>
const std::vector<Element>* ResourceRegistry::get_buffer_ptr(BufferHandle<Element> handle) const {
    const auto* store = get_buffer_store<Element>();
    return &store->slots[handle.index_].data.value();
}

template<typename Element>
std::vector<Element>* ResourceRegistry::get_buffer_ptr_mut(BufferHandle<Element> handle) {
    auto* store = get_buffer_store<Element>();
    return &store->slots[handle.index_].data.value();
}

template<typename PipelineType>
std::expected<PipelineHandle<PipelineType>, ResourceError>
ResourceRegistry::insert_pipeline(std::unique_ptr<PipelineType>&& pipeline) {
    auto index = pipeline_store_.allocate();
    auto& slot = pipeline_store_.slots[index];
    slot.data = std::move(pipeline);
    return PipelineHandle<PipelineType>(index, slot.generation);
}

template<typename PipelineType>
const PipelineType* ResourceRegistry::get_pipeline_ptr(PipelineHandle<PipelineType> handle) const {
    if (handle.index_ >= pipeline_store_.slots.size())
        return nullptr;

    const auto& slot = pipeline_store_.slots[handle.index_];
    if (slot.generation != handle.generation_ || !slot.data.has_value())
        return nullptr;

    return dynamic_cast<const PipelineType*>(slot.data.value().get());
}

template<typename PipelineType>
void ResourceRegistry::destroy_pipeline(PipelineHandle<PipelineType> handle) {
    pipeline_store_.destroy(handle.index_);
}

} // namespace renderer
