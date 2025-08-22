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
  explicit Query(World *world) : world_(world) {
    (required_.set(engine::ecs::ComponentIds::get_id<Components>()), ...);
  }

  template <typename... DisallowedComponents> 
  Query &without() {
    (disallowed_.set(engine::ecs::ComponentIds::get_id<DisallowedComponents>()), ...);
    invalidate_cache();
    return *this;
  }

  template <typename... RequiredComponents>
  Query &with() {
    (required_.set(engine::ecs::ComponentIds::get_id<RequiredComponents>()), ...);
    invalidate_cache();
    return *this;
  }

    struct Iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::tuple<Entity, Components...>;
    using reference = std::tuple<Entity, Components &...>;
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
    ensure_archetypes_cached();
    return Iterator(matching_archetypes_.begin(), matching_archetypes_.end()); 
  }

  Iterator end() { 
    ensure_archetypes_cached();
    return Iterator(matching_archetypes_.end(), matching_archetypes_.end()); 
  }

  bool empty() {
    ensure_archetypes_cached();
    return std::all_of(matching_archetypes_.begin(), matching_archetypes_.end(),
                      [](Archetype* arch) { return arch->empty(); });
  }

  std::size_t count() {
    ensure_archetypes_cached();
    std::size_t total = 0;
    for (Archetype* arch : matching_archetypes_) {
      total += arch->size();
    }
    return total;
  }

private:
  void ensure_archetypes_cached() const {
    if (!archetypes_cached_) {
      matching_archetypes_ = world_->get_matching_archetypes(required_, disallowed_);
      archetypes_cached_ = true;
    }
  }

  void invalidate_cache() {
    archetypes_cached_ = false;
    matching_archetypes_.clear();
  }

  ComponentMask required_;
  ComponentMask disallowed_;
  World *world_;
  mutable std::vector<Archetype *> matching_archetypes_;
  mutable bool archetypes_cached_ = false;
};

} // namespace engine::ecs
