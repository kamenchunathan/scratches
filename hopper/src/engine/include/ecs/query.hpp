#pragma once

#include "ecs/archetype.hpp"
#include "ecs/component.hpp"
#include "ecs/world.hpp"
#include <iterator>
#include <vector>

namespace engine::ecs {

/* For constructing queryies e.g.
 * ```cpp
 *  auto living_entities = engine::ecs::Query<Position, Health>(&world).without<Dead>();
 * ```
 * */
template<typename... Components>
class Query {
public:
    explicit Query(World* world): world_(world) {
        (required_.set(engine::ecs::ComponentIds::get_id<Components>()), ...);
    }

    /* Iterate over entities with this additional component without including it in the query
     * Useful for components used as tags
     * 
     * This can be called after iterating over the query and the caching ensures that iterating over
     * the query again after this function call refreshes the matching archetypes without having
     * to do so over every iteration.
     * I'm unsure if this is the right direction to go with for the API. It is the simplest and most
     * direct however. Alternatives would be using some type-level shenanigans such as in the bevy API
     *
     * TODO: Explore API design directions for filtering the components 
     *
     * As with and without can only ever shrink the matching archetypes components and not 
     * expand it, it should be possible to avoid refetching the archetype list from the world every
     * time and instead filter it in place. 
     * This introduces additional complications as it always shrinks the queryset on every call. 
     * Again an API design issue. Do I want to call this multiple times or is creating different 
     * queries better
     * 
     * */
    template<typename... RequiredComponents>
    Query& with() {
        (required_.set(engine::ecs::ComponentIds::get_id<RequiredComponents>()), ...);
        invalidate_cache();
        return *this;
    }

    /* Exclude entities with these components from the query */
    template<typename... DisallowedComponents>
    Query& without() {
        (disallowed_.set(engine::ecs::ComponentIds::get_id<DisallowedComponents>()), ...);
        invalidate_cache();
        return *this;
    }

    struct Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::tuple<Entity, Components...>;

        Iterator() = default;

        Iterator(
            std::vector<Archetype*>::iterator archetype_it,
            std::vector<Archetype*>::iterator archetype_end
        ):
            archetype_it_(archetype_it),
            archetype_end_(archetype_end),
            entity_idx_(0) {
            skip_empty_archetypes();
        }

        value_type operator*() const {
            Entity entity = (*archetype_it_)->get_entities()[entity_idx_];
            return std::make_tuple(
                entity,
                (*archetype_it_)->template get_component<Components>(entity_idx_)...
            );
        }

        // Not expected to be used as we return a tuple and not a pointer-like object and don't
        // anticipate this access pattern
        // Destructuring the tuple to get the components is how the iterator should be used
        auto operator->() = delete;

        Iterator& operator++() {
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

        bool operator==(const Iterator& other) const {
            return archetype_it_ == other.archetype_it_
                && (archetype_it_ == archetype_end_ || entity_idx_ == other.entity_idx_);
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

        bool operator==(std::default_sentinel_t) const {
            return archetype_it_ == archetype_end_;
        }

    private:
        void skip_empty_archetypes() {
            while (archetype_it_ != archetype_end_
                   && ((*archetype_it_)->empty() || entity_idx_ >= (*archetype_it_)->size()))
            {
                ++archetype_it_;
                entity_idx_ = 0;
            }
        }

        std::vector<Archetype*>::iterator archetype_it_;
        std::vector<Archetype*>::iterator archetype_end_;
        std::uint32_t entity_idx_ = 0;
    };

    using iterator = Iterator;
    using const_iterator = Iterator;

    Iterator begin() const {
        ensure_archetypes_cached();
        return Iterator(matching_archetypes_.begin(), matching_archetypes_.end());
    }

    Iterator end() const {
        ensure_archetypes_cached();
        return Iterator(matching_archetypes_.end(), matching_archetypes_.end());
    }

    Iterator cbegin() const {
        return begin();
    }

    Iterator cend() const {
        return end();
    }

    std::size_t size() const {
        ensure_archetypes_cached();
        std::size_t total = 0;
        for (Archetype* arch: matching_archetypes_) {
            total += arch->size();
        }
        return total;
    }

    bool empty() const {
        ensure_archetypes_cached();
        return std::all_of(
            matching_archetypes_.begin(),
            matching_archetypes_.end(),
            [](Archetype* arch) { return arch->empty(); }
        );
    }

    std::size_t count() const {
        return size();
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
    World* world_;
    mutable std::vector<Archetype*> matching_archetypes_;
    mutable bool archetypes_cached_ = false;
};

} // namespace engine::ecs
