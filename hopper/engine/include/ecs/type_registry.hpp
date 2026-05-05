#pragma once

#include <atomic>
#include <cstdint>

namespace ecs {

/// Generic registry for types to provide unique IDs at runtime.
/// @tparam Category A tag type to separate different ID spaces (e.g., Components, Stages).
template<typename Category>
class TypeRegistry {
public:
    template<typename T>
    static auto get_id() -> std::uint32_t {
        static const std::uint32_t id = count_.fetch_add(1, std::memory_order_seq_cst);
        return id;
    }

    static auto count() -> std::uint32_t {
        return count_.load(std::memory_order_relaxed);
    }

private:
    inline static std::atomic_uint32_t count_ {0};
};

} // namespace ecs
