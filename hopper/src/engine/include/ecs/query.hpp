#pragma once

#include "ecs/archetype.hpp"
#include "ecs/component.hpp"
#include "ecs/world.hpp"
#include <iterator>
#include <vector>

namespace engine::ecs {
  
template <typename... Components>
class Query {
public:
  Query(World *world) : world_(world) {
    matching_archetypes_ =
        world_->get_matching_archetypes(required_, disallowed_);
    (required_.set(engine::ecs::ComponentIds::get_id<Components>()), ...);
  };

  template <typename... DisallowedComponents> Query &without() {
    (disallowed_.set(engine::ecs::ComponentIds::get_id<DisallowedComponents>()), ...);
    return *this;
  }

  struct Iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::tuple<Entity, Components...>;
    using reference = std::tuple<Entity &, Components &...>;
    using pointer = void;

    Iterator( std::vector<Archetype *>::iterator matching_archetypes)
        : matching_archetypes_(matching_archetypes) {}

    reference operator*()   {
      std::uint32_t entity_index = (*matching_archetypes_)->get_entities()[curr_entity_id_];
      
      return std::tie(entity_index, (*matching_archetypes_)->template get_component<Components>( entity_index)...); }

    Iterator &operator++() {
      if (curr_entity_id_ < (*matching_archetypes_)->get_entities().size()) {
        ++curr_entity_id_;
      } else {
        curr_entity_id_ = 0;
        ++matching_archetypes_;
      }
      return *this;
    }
    
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const Iterator &a, const Iterator &b) {
      return a.matching_archetypes_ == b.matching_archetypes_;
    }

    friend bool operator!=(const Iterator &a, const Iterator &b) {
      return a.matching_archetypes_ != b.matching_archetypes_;
    }

  private:
    std::vector<Archetype *>::iterator matching_archetypes_;
    std::uint32_t curr_entity_id_ = 0;
  };

  Iterator begin() { return Iterator(matching_archetypes_.begin()); }

  Iterator end() { return Iterator(matching_archetypes_.end()); }

private:
  ComponentMask required_;
  ComponentMask disallowed_;
  World *world_;
  std::vector<Archetype *> matching_archetypes_;
};

} // namespace engine::ecs
