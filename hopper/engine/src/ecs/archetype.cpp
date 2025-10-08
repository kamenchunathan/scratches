#include "ecs/archetype.hpp"

namespace ecs {

void Archetype::remove_entity(std::uint32_t index) {
    assert(index < entity_indices_.size());

    // Move last entity to this position (swap and pop)
    if (index < entity_indices_.size() - 1) {
        entity_indices_[index] = entity_indices_.back();
    }
    entity_indices_.pop_back();

    // Remove components using the same strategy
    for (auto& [component_id, column]: components_) {
        column->remove_component(index);
    }
}

} // namespace ecs
