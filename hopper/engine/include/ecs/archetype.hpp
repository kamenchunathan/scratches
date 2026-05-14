#pragma once

#include <cassert>
#include <memory>
#include <unordered_map>
#include <vector>

#include "ecs/component.hpp"
#include "ecs/entity.hpp"

namespace ecs {

/* A type erased component column */
class ComponentColumn {
public:
    virtual ~ComponentColumn()                                       = default;
    virtual void* get_component_ptr(std::uint32_t index)             = 0;
    virtual const void* get_component_ptr(std::uint32_t index) const = 0;
    virtual void remove_component(std::uint32_t index)               = 0;
    virtual void reserve(std::size_t capacity)                       = 0;
};

template<typename T>
class TypedComponentColumn: public ComponentColumn {
public:
    void add_component(T component) {
        components_.emplace_back(std::move(component));
    }

    T& get_component(std::uint32_t index) {
        assert(index < components_.size());
        return components_[index];
    }

    const T& get_component(std::uint32_t index) const {
        assert(index < components_.size());
        return components_[index];
    }

    void* get_component_ptr(std::uint32_t index) override {
        return &get_component(index);
    }

    const void* get_component_ptr(std::uint32_t index) const override {
        return &get_component(index);
    }

    void remove_component(std::uint32_t index) override {
        assert(index < components_.size());
        if (index < components_.size() - 1) {
            components_[index] = std::move(components_.back());
        }
        components_.pop_back();
    }

    void reserve(std::size_t capacity) override {
        components_.reserve(capacity);
    }

    std::size_t size() const {
        return components_.size();
    }

private:
    std::vector<T> components_;
};

/* A table of entities with the same components
 * Concepturally thought of as a
 */
class Archetype {
public:
    explicit Archetype(const Signature& signature): signature_(signature) {
        entity_indices_.reserve(64);
    }

    ~Archetype() = default;

    // Non-copyable, movable
    Archetype(const Archetype&)            = delete;
    Archetype& operator=(const Archetype&) = delete;
    Archetype(Archetype&&)                 = default;
    Archetype& operator=(Archetype&&)      = default;

    template<typename T>
    T& get_component(std::uint32_t index);

    template<typename T>
    const T& get_component(std::uint32_t index) const {
        const auto* column = get_component_column<T>();
        return column->get_component(index);
    }

    template<typename... Components>
    std::uint32_t add_entity(Entity entity, Components&&... components);

    void remove_entity(std::uint32_t index);

    std::size_t size() const {
        return entity_indices_.size();
    }

    bool empty() const {
        return entity_indices_.empty();
    }

    bool matches_query(const ComponentMask& required, const ComponentMask& disallowed) const {
        return (signature_.mask & required) == required && (signature_.mask & disallowed) == 0;
    }

    const std::vector<Entity>& get_entities() const {
        return entity_indices_;
    }
    const Signature& get_signature() const {
        return signature_;
    }

private:
    Signature signature_;
    std::vector<Entity> entity_indices_;
    std::unordered_map<ComponentId, std::unique_ptr<ComponentColumn>> components_;

    template<typename T>
    TypedComponentColumn<T>* get_component_column();

    template<typename T>
    const TypedComponentColumn<T>* get_component_column() const;

    template<typename T>
    void add_component_to_column(T&& component);
};

template<typename... Components>
std::uint32_t Archetype ::add_entity(Entity entity, Components&&... components) {
    entity_indices_.push_back(entity);
    std::uint32_t index = static_cast<std::uint32_t>(entity_indices_.size() - 1);

    (add_component_to_column(std::forward<Components>(components)), ...);

    return index;
}

template<typename T>
T& Archetype::get_component(std::uint32_t index) {
    auto* column = get_component_column<T>();
    return column->get_component(index);
}

template<typename T>
TypedComponentColumn<T>* Archetype::get_component_column() {
    auto component_id = ComponentRegistry::get_id<T>();
    auto it           = components_.find(component_id);
    assert(it != components_.end() && "Component not found in archetype");
    return static_cast<TypedComponentColumn<T>*>(it->second.get());
}

template<typename T>
const TypedComponentColumn<T>* Archetype::get_component_column() const {
    auto component_id = ComponentRegistry::get_id<T>();
    auto it           = components_.find(component_id);
    assert(it != components_.end() && "Component not found in archetype");
    return static_cast<const TypedComponentColumn<T>*>(it->second.get());
}

template<typename T>
void Archetype::add_component_to_column(T&& component) {
    auto component_id = ComponentRegistry::get_id<T>();
    auto it           = components_.find(component_id);

    if (it == components_.end()) {
        auto column = std::make_unique<TypedComponentColumn<T>>();
        column->reserve(entity_indices_.capacity());
        column->add_component(std::forward<T>(component));
        components_[component_id] = std::move(column);
    } else {
        auto* typed_column = static_cast<TypedComponentColumn<T>*>(it->second.get());
        typed_column->add_component(std::forward<T>(component));
    }
}

} // namespace ecs
