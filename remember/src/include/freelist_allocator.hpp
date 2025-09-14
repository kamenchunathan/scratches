#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <new>
#include <sys/mman.h>
#include <unistd.h>

inline static size_t round_up(size_t size, size_t alignment) {
    const size_t mask = alignment - 1;
    return (size + mask) & ~mask;
}

struct Block {
    std::size_t size;
    bool allocated;

    Block* next_block() {
        return reinterpret_cast<Block*>(reinterpret_cast<std::byte*>(this) + this->size);
    }
};

struct Page {
    std::size_t size;
    Page* next;
    Page* prev;
};
// TODO: Implement Coalescing: deallocate should check for adjacent free blocks and merge them to combat
//     external fragmentation.
//  TODO: Implement Multi-Page Support: The allocator should be able to mmap new pages when the existing
//     ones are full.
//  TODO: C++ Allocator Conformance: Address rebinding, copy/move semantics, and operator== to make the
//     allocator compatible with standard library containers.
template<typename T>
class FreeListAllocator {
public:
    using value_type = T;

    FreeListAllocator() = default;

    T* allocate(std::size_t n) {
        auto page_header_size = round_up(sizeof(Page), std::max(alignof(Page), alignof(Block)));
        auto block_header_size = round_up(sizeof(Block), std::max(alignof(Block), alignof(T)));
        auto total_requested_alloc_size = block_header_size + sizeof(T) * n;
        auto min_block_size =
            round_up(sizeof(Block), std::max(alignof(Block), alignof(std::size_t)))
            + sizeof(std::size_t);

        // TODO: More comprehensive free page finding logic
        if (!page_list_) {
            std::size_t page_size = sysconf(_SC_PAGESIZE);
            void* page =
                mmap(nullptr, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
            if (!page) {
                throw std::bad_alloc();
            }
            memset(page, 0, page_size);
            page_list_ = reinterpret_cast<Page*>(page);
            page_list_->prev = nullptr;
            page_list_->next = nullptr;
            page_list_->size = page_size;

            auto start = reinterpret_cast<Block*>(
                reinterpret_cast<std::byte*>(page_list_) + page_header_size
            );
            start->allocated = false;
            start->size = page_size - page_header_size;
        }

        // Find next free block
        auto block =
            reinterpret_cast<Block*>(reinterpret_cast<std::byte*>(page_list_) + page_header_size);
        auto remaining_memory_size = page_list_->size - page_header_size;

        while (true) {
            if (!block->allocated && block->size >= total_requested_alloc_size) {
                break;
            }

            remaining_memory_size -= block->size;
            if (remaining_memory_size <= 0) {
                throw std::bad_alloc();
            }

            block = block->next_block();
        }

        // Break the block
        if (block->size > total_requested_alloc_size + min_block_size) {
            auto new_block = reinterpret_cast<Block*>(
                reinterpret_cast<std::byte*>(block) + total_requested_alloc_size
            );
            new_block->size = block->size - total_requested_alloc_size;
            new_block->allocated = false;

            block->size = total_requested_alloc_size;
        }

        block->allocated = true;
        return reinterpret_cast<T*>(reinterpret_cast<std::byte*>(block) + block_header_size);
    }

    void deallocate(T* p, std::size_t n) {
        auto block_header_size = round_up(sizeof(Block), std::max(alignof(Block), alignof(T)));
        auto total_alloc_size = block_header_size + sizeof(T) * n;

        auto block = reinterpret_cast<Block*>(reinterpret_cast<std::byte*>(p) - block_header_size);
        assert(block->size == total_alloc_size);

        block->allocated = false;
    }

private:
    Page* page_list_ = nullptr;
};
