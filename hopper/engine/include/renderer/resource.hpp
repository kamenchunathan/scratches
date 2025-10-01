#pragma once

#include <any>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace renderer {

template<typename... Resources>
struct ResourcePack {
    std::tuple<Resources...> resources;

    template<typename T>
    T& get() {
        return std::get<T>(resources);
    };

    template<typename T>
    const T& get() const {
        return std::get<T>(resources);
    };
};

class ResourceRegistry {
public:
    template<typename T, typename... Args>
    void add(Args&&... args) {
        resources_[std::type_index(typeid(T))] = std::make_shared<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    T* get() {
        auto it = resources_.find(std::type_index(typeid(T)));
        if (it == resources_.end()) {
            return nullptr;
        }
        auto* ptr = std::any_cast<std::shared_ptr<T>>(&it->second);
        return ptr ? ptr->get() : nullptr;
    }

    template<typename T>
    const T* get() const {
        auto it = resources_.find(std::type_index(typeid(T)));
        if (it == resources_.end()) {
            return nullptr;
        }
        const auto* ptr = std::any_cast<std::shared_ptr<T>>(&it->second);
        return ptr ? ptr->get() : nullptr;
    }

    template<typename... RequiredResources>
    auto extract() const {
        return ResourcePack<RequiredResources...> { std::make_tuple(*get<RequiredResources>()...) };
    }

private:
    std::unordered_map<std::type_index, std::any> resources_;
};

} // namespace renderer
