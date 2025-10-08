#pragma once

#include "archetype.hpp"
#include "component.hpp"
#include "ecs/resource.hpp"
#include "entity.hpp"
#include <any>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ecs {

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
    std::optional<ResourceType&> get_resource();

    template<typename ResourceType>
    std::optional<const ResourceType&> get_resource() const;

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
    std::vector<std::unique_ptr<std::any>> resources_;
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
    ResourceId id = ResourceIds::get_id<ResourceType>();
    if (id >= resources_.size())
        resources_.resize(id + 1);
    resources_[id] = std::make_unique<std::any>(std::forward<ResourceType>(res));
}

template<typename ResourceType>
std::optional<ResourceType&> World::get_resource() {
    ResourceId id = ResourceIds::get_id<ResourceType>();
    if (id >= resources_.size())
        return std::nullopt;

    if (auto p = std::any_cast<ResourceType&>(*resources_[id]))
        return p;

    return std::nullopt;
}

template<typename ResourceType>
std::optional<const ResourceType&> World::get_resource() const {
    ResourceId id = ResourceIds::get_id<ResourceType>();
    if (id >= resources_.size())
        return std::nullopt;

    if (auto p = std::any_cast<ResourceType&>(*resources_[id]))
        return p;

    return std::nullopt;
}

} // namespace ecs
