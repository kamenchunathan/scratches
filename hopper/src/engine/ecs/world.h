#include "component.h"
#include "entity.h"
#include <cstdio>
#include <memory>
#include <unordered_map>

#include "storage/archetype.h"

class World {
public:
  template <typename... Components>
  Entity create_entity(Components &&...components) {
    Entity entity = next_entity_id++;

    Signature signature;
    (signature.mask.set(ComponentIds::get_id<Components>()), ...);

    auto arch = find_or_create_archetype(signature);
    arch->add_entity(entity, components...);
    m_entity_map.insert({entity, std::make_pair(arch, arch->size() - 1)});
    std::printf("entity %u index %u signature %u \n", entity, arch->size(),
                signature.mask);

    return entity;
  }
  void destroy_entity(Entity entity) {
    // assert(m_entity_map.count(entity)); // More appropriate check, but not
    // strictly necessary for correctness

    auto it = m_entity_map.find(entity);
    if (it == m_entity_map.end()) {
      return;
    }
    auto [archetype, index] = it->second;
    archetype->remove_entity(index);
    m_entity_map.erase(it);
  }

  template <typename... Components, typename Func> void each(Func &&func) {
    std::vector<Archetype *> archs = query<Components...>();

    for (auto arch : archs) {
      if constexpr (sizeof...(Components) == 0) {
        for (auto entity : arch->entity_indices) {

          func(entity);
        }
      } else {
        for (auto entity : arch->entity_indices) {

          func(entity, arch->template get_component<Components>(entity)...);
        }
      }
    }
  }
  template <typename... Components> std::vector<Archetype *> query() {
    ComponentMask query_mask;
    (query_mask.set(ComponentIds::get_id<Components>()), ...);

    std::vector<Archetype *> matching_archetypes;
    for (auto &[_, archetype] : m_archetypes) {
      if (archetype->matches_query(query_mask)) {
        matching_archetypes.push_back(archetype.get());
      }
    }

    return matching_archetypes;
  }

private:
  std::uint32_t next_entity_id;
  std::unordered_map<Signature, std::unique_ptr<Archetype>, SignatureHash>
      m_archetypes;
  std::unordered_map<Entity, std::pair<Archetype *, std::uint32_t>>
      m_entity_map;

  // Archetype *find_or_create_archetype(Signature);
  Archetype *find_or_create_archetype(Signature signature) {
    // SAFETY? this reference is only ever used by the world hence returning a
    // reference to the archetype should be okay as no references will outlive
    // the world
    auto it = m_archetypes.find(signature);
    if (it == m_archetypes.end()) {
      auto archetype = std::make_unique<Archetype>(signature);
      auto [inserted, success] =
          m_archetypes.insert({signature, std::move(archetype)});
      return inserted->second.get();
    } else {
      return it->second.get();
    }
  }
};
