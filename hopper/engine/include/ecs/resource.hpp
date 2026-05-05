#pragma once

#include <cstdint>

#include "ecs/type_registry.hpp"

namespace ecs {

using ResourceId = std::uint32_t;

struct ResourceCategory {};

using ResourceRegistry = TypeRegistry<ResourceCategory>;

/// Separate Resource ids generator for resources.
class ResourceIds {
public:
    template<typename T>
    static ResourceId get_id() {
        return ResourceRegistry::get_id<T>();
    }
};

} // namespace ecs
