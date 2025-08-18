#pragma once

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../component.h"
#include "../entity.h"

class ComponentColumn {
public:
  virtual void *get_component_ptr(Entity index) = 0;
  virtual const void *get_component_ptr(Entity index) const = 0;
  virtual void remove_component(Entity index) = 0;
  virtual ~ComponentColumn() = default;
};

template <typename T> class TypedComponentColumn : public ComponentColumn {
  std::vector<T> components;

public:
  void add_component(const T &component) { components.push_back(component); }

  T &get_component(std::uint32_t index) { return components[index]; }

  void *get_component_ptr(Entity entity) override {
    return &components[entity];
  }

  const void *get_component_ptr(Entity entity) const override {
    return &components[entity];
  }

  void remove_component(Entity entity) override {
    if (entity < components.size() - 1) {
      components[entity] = std::move(components.back());
    }
    components.pop_back();
  }
};

/* An archtype is table of entities with the same components where components
 * are columns. Created on each new unique set of components */
class Archetype {
public:
  Signature signature;
  std::vector<Entity> entity_indices;
  std::unordered_map<ComponentId, std::unique_ptr<ComponentColumn>> components;

  Archetype(const Signature &signature) : signature(signature) {}

  template <typename T> T &get_component(std::uint32_t index) {
    TypedComponentColumn<T> *col = get_component_column<T>();
    return col->get_component(index);
  }

  template <typename T> TypedComponentColumn<T> *get_component_column() {
    // TODO: static cast required here
    auto component_id = ComponentIds::get_id<T>();
    auto it = components.find(component_id);
    if (it == components.end()) {
      // TODO: panic: This should never happen
      printf("Component not in archetype");
      exit(1);
    }

    return static_cast<TypedComponentColumn<T> *>(it->second.get());
  }

  std::size_t size() const { return entity_indices.size(); }

  bool matches_query(const ComponentMask &query_mask) const {
    return (signature.mask & query_mask) == query_mask;
  }

  template <typename... Components>
  void add_entity(Entity entity, Components... components) {
    entity_indices.push_back(entity);
    (add_component_to_column(components), ...);
  }

  void remove_entity(Entity index) {
    // NOTE: consistency dependent on
    //  1. Full Completion of all add/removal operations across all columns.
    //  Partial changes mean that entities will contain the wrong components
    //  which may be hard to debug
    //  2. Consitent method used to add/remove entities across the entity
    //  columns, entity->archetype map in the world / ArchetypeManager.
    //  Currently this is guaranteed as all of these happen using a swap and pop
    //  strategy to maintain a dense structure but this implicit relationship
    //  looks a prime cause for bugs
    //  3. Poses problems for dynamic addition / removal of individual
    //  components
    //
    //  TODO:
    //   Change archetype storage implementation to be generational where
    //   archetypes and entities store their generation which is updated on
    //   component removal using either the high and low bits of a u64 or a
    //   defined structure e.g.
    //   ```c++
    //      struct EntityHandle {
    //        uint32_t id;
    //        uint32_t generation;
    //      }
    //   ```

    assert(index < entity_indices.size());
    if (index < entity_indices.size() - 1) {
      entity_indices[index] = entity_indices.back();
      entity_indices.pop_back();
    } else {
      entity_indices.pop_back();
    }

    for (auto &[component_id, column] : components) {
      column->remove_component(index);
    }
  }

private:
  template <typename T> void add_component_to_column(T component) {
    auto component_id = ComponentIds::get_id<T>();
    auto it = components.find(component_id);
    if (it == components.end()) {
      auto column = std::make_unique<TypedComponentColumn<T>>();
      column->add_component(std::forward<T>(component));
      components.insert({component_id, std::move(column)});
    } else {
      auto typed_column =
          static_cast<TypedComponentColumn<T> *>(it->second.get());
      typed_column->add_component(std::forward<T>(component));
    }
  }
};
