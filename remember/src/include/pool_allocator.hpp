#include <cassert>
#include <cstddef>
#include <memory>

template<typename T, typename Inner = std::allocator<T>>
class PoolAllocator {
    using ByteAlloc = typename std::allocator_traits<Inner>::template rebind_alloc<std::byte>;

public:
    using value_type = T;

    explicit PoolAllocator(std::size_t pool_size, const Inner& inner):
        pool_size_(pool_size),
        inner_alloc_(inner) {
        ByteAlloc byte_alloc(inner_alloc_);

        pool_ = byte_alloc.allocate(pool_size_);
        freelist_ = reinterpret_cast<Block*>(pool_);
        freelist_->next = nullptr;

        std::size_t current = 1;

        const size_t alignment = std::max(alignof(T), alignof(Block));
        const size_t unaligned_block_size = header_size + sizeof(T);
        const size_t block_size = (unaligned_block_size + alignment - 1) & ~(alignment - 1);

        while ((current + 1) * block_size <= pool_size_) {
            Block* block = reinterpret_cast<Block*>(pool_ + (current * block_size));

            block->next = freelist_;
            freelist_ = block;

            current++;
        }
    }

    ~PoolAllocator() {
        ByteAlloc byte_alloc(inner_alloc_);
        byte_alloc.deallocate(pool_, pool_size_);
    }

    T* allocate(std::size_t n) {
        if (!freelist_) {
            throw std::bad_alloc();
        }

        if (n != 1) {
            throw std::bad_alloc();
        }

        auto block = freelist_;
        freelist_ = freelist_->next;
        block->next = nullptr;

        return reinterpret_cast<T*>(static_cast<std::byte*>(block) + header_size);
    }

    void deallocate(T* data, std::size_t) noexcept {
        Block* block = reinterpret_cast<Block*>(static_cast<std::byte*>(data) - header_size);

        block->next = freelist_;
        freelist_ = block;
    }

private:
    struct Block {
        Block* next;
    };

    const size_t header_size = (sizeof(Block) + std::max(alignof(T), alignof(Block)) - 1)
        & ~(std::max(alignof(T), alignof(Block)) - 1);

    Block* freelist_ = nullptr;
    Inner inner_alloc_;
    std::byte* pool_ = nullptr;
    std::size_t pool_size_ = 4096;
};

template<typename T, typename U, typename Inner>
bool operator==(const PoolAllocator<T, Inner>& a, const PoolAllocator<U, Inner>& b) {
    return a.pool_ == b.pool_;
}

template<typename T, typename U, typename Inner>
bool operator!=(const PoolAllocator<T, Inner>& a, const PoolAllocator<U, Inner>& b) {
    return !(a == b);
}
