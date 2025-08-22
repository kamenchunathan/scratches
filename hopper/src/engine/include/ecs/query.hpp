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
    (required_.set(engine::ecs::ComponentIds::get_id<Components>()), ...);
    matching_archetypes_ = world_->get_matching_archetypes(required_, disallowed_);
  }

  template <typename... DisallowedComponents> 
  Query &without() {
    (disallowed_.set(engine::ecs::ComponentIds::get_id<DisallowedComponents>()), ...);
    matching_archetypes_ = world_->get_matching_archetypes(required_, disallowed_);
    return *this;
  }

  struct Iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::tuple<Entity, Components...>;
    using reference = std::tuple<Entity &, Components &...>;
    using pointer = void;

    Iterator(std::vector<Archetype *>::iterator archetype_it,
             std::vector<Archetype *>::iterator archetype_end)
        : archetype_it_(archetype_it), archetype_end_(archetype_end), entity_idx_(0) {
      skip_empty_archetypes();
    }

    reference operator*() {
      Entity entity = (*archetype_it_)->get_entities()[entity_idx_];
      return std::tie(entity, (*archetype_it_)->template get_component<Components>(entity_idx_)...);
    }

    Iterator &operator++() {
      ++entity_idx_;
      
      // If we've gone past the current archetype's entities, move to next archetype
      if (archetype_it_ != archetype_end_ && entity_idx_ >= (*archetype_it_)->size()) {
        ++archetype_it_;
        entity_idx_ = 0;
        skip_empty_archetypes();
      }
      
      return *this;
    }
    
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const Iterator &a, const Iterator &b) {
      return a.archetype_it_ == b.archetype_it_ && 
             (a.archetype_it_ == a.archetype_end_ || a.entity_idx_ == b.entity_idx_);
    }

    friend bool operator!=(const Iterator &a, const Iterator &b) {
      return !(a == b);
    }

  private:
    void skip_empty_archetypes() {
      while (archetype_it_ != archetype_end_ && 
             ((*archetype_it_)->empty() || entity_idx_ >= (*archetype_it_)->size())) {
        ++archetype_it_;
        entity_idx_ = 0;
      }
    }

    std::vector<Archetype *>::iterator archetype_it_;
    std::vector<Archetype *>::iterator archetype_end_;
    std::uint32_t entity_idx_;
  };

  Iterator begin() { 
    return Iterator(matching_archetypes_.begin(), matching_archetypes_.end()); 
  }

  Iterator end() { 
    return Iterator(matching_archetypes_.end(), matching_archetypes_.end()); 
  }

private:
  ComponentMask required_;
  ComponentMask disallowed_;
  World *world_;
  std::vector<Archetype *> matching_archetypes_;
};

} // namespace engine::ecs
