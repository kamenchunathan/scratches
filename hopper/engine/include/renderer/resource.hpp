#pragma once

#include <any>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "renderer/texture.hpp"

namespace renderer {

template<typename, typename Tag>
struct ResourceId {
private:
    ResourceId(std::uint32_t index, std::uint32_t generation):
        index_(index),
        generation_(generation) {}

    std::uint32_t index_;
    std::uint32_t generation_;
};

struct BuffferTag {};
struct PipelineTag {};
struct TextureTag {};

class ResourceManager {
public:
    template<typename T>
    ResourceId<T, BuffferTag> create_buffer(T&& resource);

    template<typename T>
    ResourceId<T, PipelineTag> create_pipeline(T&& resource);

    // template<typename T>
    // ResourceId<T> create(T&& resource);

    // template<typename T>
    // void destroy(ResourceId<T> id);
    //
    // template<typename T>
    // bool is_valid(ResourceId<T> id) const;
    //
    // template<typename T>
    // T& get(ResourceId<T> id);

private:
    // Internal registries for textures, buffers, etc.
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

} // namespace renderer
