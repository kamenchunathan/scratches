#pragma once

#include "component.hpp"
#include "entity.hpp"
#include <cassert>
#include <memory>
#include <unordered_map>
#include <vector>

namespace engine::ecs {
    class ComponentColumn {
    public:
        virtual ~ComponentColumn() = default;
        virtual void* get_component_ptr(std::uint32_t index) = 0;
        virtual const void* get_component_ptr(std::uint32_t index) const = 0;
        virtual void remove_component(std::uint32_t index) = 0;
        virtual void reserve(std::size_t capacity) = 0;
    };
    
    template<typename T>
    class TypedComponentColumn : public ComponentColumn {
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
        
        std::size_t size() const { return components_.size(); }
        
    private:
        std::vector<T> components_;
    };
    
    class Archetype {
    public:
        explicit Archetype(const Signature& signature) : signature_(signature) {
            entity_indices_.reserve(64); // Reserve some initial capacity
        }
        
        ~Archetype() = default;
        
        // Non-copyable, movable
        Archetype(const Archetype&) = delete;
        Archetype& operator=(const Archetype&) = delete;
        Archetype(Archetype&&) = default;
        Archetype& operator=(Archetype&&) = default;
        
        template<typename T>
        T& get_component(std::uint32_t index) {
            auto* column = get_component_column<T>();
            return column->get_component(index);
        }
        
        template<typename T>
        const T& get_component(std::uint32_t index) const {
            const auto* column = get_component_column<T>();
            return column->get_component(index);
        }
        
        template<typename... Components>
        std::uint32_t add_entity(Entity entity, Components&&... components) {
            entity_indices_.push_back(entity);
            std::uint32_t index = static_cast<std::uint32_t>(entity_indices_.size() - 1);
            
            (add_component_to_column(std::forward<Components>(components)), ...);
            
            return index;
        }
        
        void remove_entity(std::uint32_t index) {
            assert(index < entity_indices_.size());
            
            // Move last entity to this position (swap and pop)
            if (index < entity_indices_.size() - 1) {
                entity_indices_[index] = entity_indices_.back();
            }
            entity_indices_.pop_back();
            
            // Remove components using the same strategy
            for (auto& [component_id, column] : components_) {
                column->remove_component(index);
            }
        }
        
        std::size_t size() const { return entity_indices_.size(); }
        bool empty() const { return entity_indices_.empty(); }
        
        bool matches_query(const ComponentMask& query_mask) const {
            return (signature_.mask & query_mask) == query_mask;
        }
        
        const std::vector<Entity>& get_entities() const { return entity_indices_; }
        const Signature& get_signature() const { return signature_; }
        
    private:
        template<typename T>
        TypedComponentColumn<T>* get_component_column() {
            auto component_id = ComponentIds::get_id<T>();
            auto it = components_.find(component_id);
            assert(it != components_.end() && "Component not found in archetype");
            return static_cast<TypedComponentColumn<T>*>(it->second.get());
        }
        
        template<typename T>
        const TypedComponentColumn<T>* get_component_column() const {
            auto component_id = ComponentIds::get_id<T>();
            auto it = components_.find(component_id);
            assert(it != components_.end() && "Component not found in archetype");
            return static_cast<const TypedComponentColumn<T>*>(it->second.get());
        }
        
        template<typename T>
        void add_component_to_column(T&& component) {
            auto component_id = ComponentIds::get_id<T>();
            auto it = components_.find(component_id);
            
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
        
        Signature signature_;
        std::vector<Entity> entity_indices_;
        std::unordered_map<ComponentId, std::unique_ptr<ComponentColumn>> components_;
    };
}
