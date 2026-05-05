#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>

#include "ecs/type_registry.hpp"

namespace ecs {

struct ComponentRegistryTag {};

// TODO: Add a dynamic component list without a set limit
constexpr std::uint32_t MAX_COMPONENTS = 128;

using ComponentMask = std::bitset<MAX_COMPONENTS>;
using ComponentId = std::uint32_t;

struct Signature {
    std::bitset<MAX_COMPONENTS> mask;

    bool operator==(const Signature& other) const {
        return mask == other.mask;
    }
};

struct SignatureHash {
    std::size_t operator()(const Signature& s) const {
        return std::hash<std::bitset<MAX_COMPONENTS>> {}(s.mask);
    }
};

/// Wrapper over the generic registry to enforce the maximum number of components supported
class ComponentRegistry {
public:
    template<typename T>
    static ComponentId get_id() {
        ComponentId id = TypeRegistry<ComponentRegistryTag>::get_id<T>();
        if (id >= MAX_COMPONENTS)
            throw std::runtime_error("Exceeded max components limit");
        return id;
    }
};

} // namespace ecs
