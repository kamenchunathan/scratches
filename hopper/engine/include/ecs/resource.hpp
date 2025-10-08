#pragma once

#include <atomic>
#include <cstdint>

namespace ecs {

using ResourceId = std::uint32_t;

/* Separate Resource ids generator for resources so as not to affect the MAX_COMPONENTS
 * Essentially a copy of the ComponentIds struct
 */
class ResourceIds {
public:
    template<typename T>
    static ResourceId get_id() {
        // Static local variables with a non-constexpr initializer are reinitialized the first
        // time the variable definition is encountered. The definition is skipped on subsequent
        // calls, so no futher reinitialization happens
        static ResourceId id = count_.fetch_add(1, std::memory_order_seq_cst);
        return id;
    }

private:
    inline static std::atomic<ResourceId> count_;
};

} // namespace ecs
