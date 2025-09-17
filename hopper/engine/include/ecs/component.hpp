#pragma once

#include <atomic>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace ecs {
constexpr std::uint32_t MAX_COMPONENTS = 64;

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
        static ComponentId id = counter_.fetch_add(1, std::memory_order_seq_cst);
        return id;
    }

private:
    // NO need for this to be atomic currently but it may be needed to make the
    // ECS system threadsafe for networking later on
    static inline std::atomic<ComponentId> counter_ { 0 };
};
} // namespace ecs
