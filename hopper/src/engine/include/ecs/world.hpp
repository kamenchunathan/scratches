#pragma once

#include "archetype.hpp"
#include "component.hpp"
#include "entity.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace engine::ecs {
class World {
public:
  World() = default;
  ~World() = default;

  World(const World &) = delete;
  World &operator=(const World &) = delete;
  World(World &&) = default;
  World &operator=(World &&) = default;

  template <typename... Components>
  Entity create_entity(Components &&...components) {
    Entity entity = next_entity_id_++;

    Signature signature;
    (signature.mask.set(ComponentIds::get_id<Components>()), ...);

    auto *archetype = find_or_create_archetype(signature);
    std::uint32_t index =
        archetype->add_entity(entity, std::forward<Components>(components)...);

    entity_map_[entity] = std::make_pair(archetype, index);

    return entity;
  }

  void destroy_entity(Entity entity) {
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

  bool is_entity_valid(Entity entity) const {
    return entity_map_.find(entity) != entity_map_.end();
  }

  template <typename... Components, typename Func> void each(Func &&func) {
    auto archetypes = query<Components...>();

    for (auto *archetype : archetypes) {
      const auto &entities = archetype->get_entities();

      if constexpr (sizeof...(Components) == 0) {
        for (std::size_t i = 0; i < entities.size(); ++i) {
          func(entities[i]);
        }
      } else {
        for (std::size_t i = 0; i < entities.size(); ++i) {
          func(entities[i], archetype->template get_component<Components>(
                                static_cast<std::uint32_t>(i))...);
        }
      }
    }
  }

  template <typename... Components, typename Func>
  void each(Func &&func) const {
    auto archetypes = query<Components...>();

    for (const auto *archetype : archetypes) {
      const auto &entities = archetype->get_entities();

      if constexpr (sizeof...(Components) == 0) {
        for (std::size_t i = 0; i < entities.size(); ++i) {
          func(entities[i]);
        }
      } else {
        for (std::size_t i = 0; i < entities.size(); ++i) {
          func(entities[i], archetype->template get_component<Components>(
                                static_cast<std::uint32_t>(i))...);
        }
      }
    }
  }

  template <typename... Components> std::vector<Archetype *> query() {
    ComponentMask query_mask;
    (query_mask.set(ComponentIds::get_id<Components>()), ...);

    std::vector<Archetype *> matching_archetypes;
    for (auto &[signature, archetype] : archetypes_) {
      if (archetype->matches_query(query_mask)) {
        matching_archetypes.push_back(archetype.get());
      }
    }

    return matching_archetypes;
  }

  template <typename... Components>
  std::vector<const Archetype *> query() const {
    ComponentMask query_mask;
    (query_mask.set(ComponentIds::get_id<Components>()), ...);

    std::vector<const Archetype *> matching_archetypes;
    for (const auto &[signature, archetype] : archetypes_) {
      if (archetype->matches_query(query_mask)) {
        matching_archetypes.push_back(archetype.get());
      }
    }

    return matching_archetypes;
  }

  std::size_t entity_count() const { return entity_map_.size(); }
  std::size_t archetype_count() const { return archetypes_.size(); }

private:
  Archetype *find_or_create_archetype(const Signature &signature) {
    auto it = archetypes_.find(signature);
    if (it == archetypes_.end()) {
      auto archetype = std::make_unique<Archetype>(signature);
      auto *ptr = archetype.get();
      archetypes_[signature] = std::move(archetype);
      return ptr;
    }
    return it->second.get();
  }

  std::uint32_t next_entity_id_{1};
  std::unordered_map<Signature, std::unique_ptr<Archetype>, SignatureHash>
      archetypes_;
  std::unordered_map<Entity, std::pair<Archetype *, std::uint32_t>> entity_map_;
};
} // namespace engine::ecs
