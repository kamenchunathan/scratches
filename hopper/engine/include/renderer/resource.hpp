#pragma once

#include <any>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace renderer {




class ShaderResourceRegistry {
public:
    template<typename T, typename Args>
    void bind(Args&& args) {
        resources_[std::type_index(typeid(T))] = std::make_unique<T>(std::forward<Args>(args));
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
        auto* ptr = std::any_cast<std::unique_ptr<T>>(&it->second);
        return ptr ? ptr->get() : nullptr;
    }

    template<typename T>
    const T* get() const {
        auto it = resources_.find(std::type_index(typeid(T)));
        if (it == resources_.end()) {
            return nullptr;
        }
        const auto* ptr = std::any_cast<std::unique_ptr<T>>(&it->second);
        return ptr ? ptr->get() : nullptr;
    }

    template<typename... RequiredResources>
    auto extract() const {
        return std::make_tuple(*get<RequiredResources>()...);
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<std::any>> resources_;
};

} // namespace renderer
