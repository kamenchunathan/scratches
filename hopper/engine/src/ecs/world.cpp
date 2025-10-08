#include "ecs/world.hpp"

namespace ecs {

bool World::is_entity_valid(Entity entity) const {
    return entity_map_.find(entity) != entity_map_.end();
}

std::vector<Archetype*>
World::get_matching_archetypes(ComponentMask required, ComponentMask disallowed) {
    std::vector<Archetype*> matching_archetypes;
    for (const auto& [signature, archetype]: archetypes_) {
        if (archetype->matches_query(required, disallowed)) {
            matching_archetypes.push_back(archetype.get());
        }
    }

    return matching_archetypes;
}
Archetype* World::find_or_create_archetype(const Signature& signature) {
    auto it = archetypes_.find(signature);
    if (it == archetypes_.end()) {
        auto archetype = std::make_unique<Archetype>(signature);
        auto* ptr = archetype.get();
        archetypes_[signature] = std::move(archetype);
        return ptr;
    }
    return it->second.get();
}
} // namespace ecs
