#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "archetype.hpp"
#include "component.hpp"
#include "ecs/resource.hpp"
#include "entity.hpp"

namespace ecs {

namespace detail {
    /* Type Erased ResourceStore to allow move only types
     * Using std::any had downsides among which included not allowing move only types
     */
    class IResourceStore {
    public:
        virtual ~IResourceStore() = default;
    };

    template<typename T>
    class ResourceStore: public IResourceStore {
    public:
        explicit ResourceStore(const T& resource): resource_(resource) {}
        explicit ResourceStore(T&& resource): resource_(std::move(resource)) {}

        T& get() {
            return resource_;
        }

        const T& get() const {
            return resource_;
        }

    private:
        T resource_;
    };

} // namespace detail

class World {
public:
    World() = default;
    ~World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = default;
    World& operator=(World&&) = default;

    template<typename... Components>
    Entity spawn(Components&&... components);

    template<typename... Components>
    void destroy(Entity entity);

    bool is_entity_valid(Entity entity) const;

    template<typename ResourceType>
    void insert_resource(ResourceType&& res);

    template<typename ResourceType>
    ResourceType* get_resource();

    template<typename ResourceType>
    const ResourceType* get_resource() const;

    template<typename ResourceType>
    bool has_resource() const;

    template<typename ResourceType>
    void remove_resource();

    std::vector<Archetype*>
    get_matching_archetypes(ComponentMask required, ComponentMask disallowed);

    std::size_t entity_count() const {
        return entity_map_.size();
    }

    std::size_t archetype_count() const {
        return archetypes_.size();
    }

private:
    Archetype* find_or_create_archetype(const Signature& signature);

    std::uint32_t next_entity_id_ {1};
    std::unordered_map<Signature, std::unique_ptr<Archetype>, SignatureHash> archetypes_;
    std::unordered_map<Entity, std::pair<Archetype*, std::uint32_t>> entity_map_;
    std::vector<std::unique_ptr<detail::IResourceStore>> resources_;
};

template<typename... Components>
Entity World::spawn(Components&&... components) {
    Entity entity = next_entity_id_++;

    Signature signature;
    (signature.mask.set(ComponentIds::get_id<Components>()), ...);

    auto* archetype = find_or_create_archetype(signature);
    std::uint32_t index = archetype->add_entity(entity, std::forward<Components>(components)...);

    entity_map_[entity] = std::make_pair(archetype, index);

    return entity;
}

template<typename... Components>
void World::destroy(Entity entity) {
    auto it = entity_map_.find(entity);
    // Entity doesn't exist
    if (it == entity_map_.end()) {
        return;
    }

    auto [archetype, index] = it->second;

    // If not the last entity in archetype, update the moved entity's mapping
    if (index < archetype->size() - 1) {
        Entity moved_entity = archetype->get_entities().back();
        entity_map_[moved_entity].second = index;
    }

    archetype->remove_entity(index);
    entity_map_.erase(it);
}

template<typename ResourceType>
void World::insert_resource(ResourceType&& res) {
    using StoredType = std::remove_cvref_t<ResourceType>;
    ResourceId id = ResourceIds::get_id<StoredType>();

    if (id >= resources_.size()) {
        resources_.resize(id + 1);
    }

    // Create a new holder with the resource (supports both copy and move)
    resources_[id] =
        std::make_unique<detail::ResourceStore<StoredType>>(std::forward<ResourceType>(res));
}

template<typename ResourceType>
ResourceType* World::get_resource() {
    using StoredType = std::remove_cvref_t<ResourceType>;
    ResourceId id = ResourceIds::get_id<StoredType>();

    if (id >= resources_.size() || !resources_[id]) {
        return nullptr;
    }

    // Dynamic cast to the correct holder type and extract the resource
    auto* holder = dynamic_cast<detail::ResourceStore<StoredType>*>(resources_[id].get());
    return holder ? &holder->get() : nullptr;
}

template<typename ResourceType>
const ResourceType* World::get_resource() const {
    using StoredType = std::remove_cvref_t<ResourceType>;
    ResourceId id = ResourceIds::get_id<StoredType>();

    if (id >= resources_.size() || !resources_[id]) {
        return nullptr;
    }

    const auto* holder =
        dynamic_cast<const detail::ResourceStore<StoredType>*>(resources_[id].get());
    return holder ? &holder->get() : nullptr;
}

template<typename ResourceType>
bool World::has_resource() const {
    using StoredType = std::remove_cvref_t<ResourceType>;
    ResourceId id = ResourceIds::get_id<StoredType>();

    if (id >= resources_.size() || !resources_[id]) {
        return false;
    }

    return dynamic_cast<const detail::ResourceStore<StoredType>*>(resources_[id].get()) != nullptr;
}

template<typename ResourceType>
void World::remove_resource() {
    using StoredType = std::remove_cvref_t<ResourceType>;
    ResourceId id = ResourceIds::get_id<StoredType>();

    if (id < resources_.size()) {
        resources_[id].reset();
    }
}

} // namespace ecs
