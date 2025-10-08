#pragma once

#include <atomic>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>

namespace ecs {

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

class ComponentIds {
public:
    template<typename T>
    static ComponentId get_id() {
        // Static local variables with a non-constexpr initializer are reinitialized the first
        // time the variable definition is encountered. The definition is skipped on subsequent
        // calls, so no futher reinitialization happens
        static ComponentId id = counter_.fetch_add(1, std::memory_order_seq_cst);
        if (id >= MAX_COMPONENTS)
            throw std::runtime_error("Exceeded max components limit");
        return id;
    }

private:
    // NO need for this to be atomic currently but it may be needed to make the
    // ECS system threadsafe for networking later on
    static inline std::atomic<ComponentId> counter_ {0};
};

} // namespace ecs
