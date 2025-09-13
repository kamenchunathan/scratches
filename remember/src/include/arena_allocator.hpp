#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>

/**
 * @brief An allocator that manages a fixed-size memory arena.
 * Copies of the allocator share the same arena.
 *
 * @tparam T The type of object to allocate.
 * @tparam Inner The underlying allocator to use for acquiring the arena itself.
 */
template<typename T, typename Inner = std::allocator<std::byte>>
class ArenaAllocator {
public:
    using value_type = T;

    template<typename U>
    struct rebind {
        using other = ArenaAllocator<U, Inner>;
    };

    /**
     * @brief Construct a new Arena Allocator.
     * @param arena_size The size of the memory arena in bytes.
     * @param inner The inner allocator instance to use for creating the arena.
     */
    explicit ArenaAllocator(std::size_t arena_size = 4096, const Inner& inner = Inner()):
        arena_(std::make_shared<Arena>(arena_size, inner)) {}

    ArenaAllocator(const ArenaAllocator& other) noexcept = default;
    ArenaAllocator& operator=(const ArenaAllocator& other) noexcept = default;

    template<typename U>
    ArenaAllocator(const ArenaAllocator<U, Inner>& other) noexcept: arena_(other.arena_) {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(arena_->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T*, std::size_t) noexcept {}

private:
    struct Arena {
        using ByteAlloc = typename std::allocator_traits<Inner>::template rebind_alloc<std::byte>;

        Arena(std::size_t arena_size, const Inner& inner):
            arena_size_(arena_size),
            inner_alloc_(inner) {
            ByteAlloc byte_alloc(inner_alloc_);
            arena_start_ = byte_alloc.allocate(arena_size_);
        }

        ~Arena() {
            ByteAlloc byte_alloc(inner_alloc_);
            byte_alloc.deallocate(arena_start_, arena_size_);
        }

        Arena(const Arena&) = delete;
        Arena& operator=(const Arena&) = delete;

        void* allocate(std::size_t n, std::size_t alignment) {
            const size_t aligned_offset = (offset_ + alignment - 1) & ~(alignment - 1);

            if (aligned_offset + n > arena_size_) {
                throw std::bad_alloc();
            }

            offset_ = aligned_offset + n;
            return arena_start_ + aligned_offset;
        }

        std::size_t arena_size_;
        std::size_t offset_ = 0;
        Inner inner_alloc_;
        std::byte* arena_start_ = nullptr;
    };

    std::shared_ptr<Arena> arena_;
};

template<typename T, typename U, typename Inner>
bool operator==(const ArenaAllocator<T, Inner>& a, const ArenaAllocator<U, Inner>& b) {
    return a.arena_ == b.arena_;
}

template<typename T, typename U, typename Inner>
bool operator!=(const ArenaAllocator<T, Inner>& a, const ArenaAllocator<U, Inner>& b) {
    return !(a == b);
}
