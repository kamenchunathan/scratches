#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>

class AllocationTracker {
public:
    AllocationTracker(const AllocationTracker&) = delete;
    AllocationTracker& operator=(const AllocationTracker&) = delete;

    static AllocationTracker& instance() {
        static AllocationTracker tracker;
        return tracker;
    }

    void record_allocation(std::size_t n, std::size_t align) {
        std::lock_guard<std::mutex> lock(mutex_);
        total_allocations_ += 1;
        total_allocated_ += n;
        std::printf("[ALLOC] size: %lu, align: %lu\n", n, align);
    }

    void record_deallocation(std::size_t n, std::size_t align) {
        std::lock_guard<std::mutex> lock(mutex_);
        total_deallocations_ += 1;
        total_deallocated_ += n;
        std::printf("[DEALLOC] size: %lu, align: %lu\n", n, align);
    }

    void print_summary() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::printf(
            "Total allocations: %lu\nTotal memory allocated: %lu\n"
            "Total deallocations: %lu\nTotal memory deallocated: %lu\n",
            total_allocations_,
            total_allocated_,
            total_deallocations_,
            total_deallocated_
        );
    }

private:
    AllocationTracker() = default;
    mutable std::mutex mutex_;
    size_t total_allocations_ = 0;
    size_t total_allocated_ = 0;
    size_t total_deallocations_ = 0;
    size_t total_deallocated_ = 0;
};

template<typename T, typename Inner = std::allocator<T>>
class DebugAllocator {
public:
    using value_type = T;

    template<typename U>
    struct rebind {
        using other =
            DebugAllocator<U, typename std::allocator_traits<Inner>::template rebind_alloc<U>>;
    };

    DebugAllocator() = default;
    explicit DebugAllocator(const Inner& inner): inner_(inner) {}
    template<typename U, typename OtherUpstream>
    DebugAllocator(const DebugAllocator<U, OtherUpstream>& other): inner_(other.inner()) {}

    T* allocate(std::size_t n) {
        AllocationTracker::instance().record_allocation(n * sizeof(T), alignof(T));
        return inner_.allocate(n);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        AllocationTracker::instance().record_deallocation(n * sizeof(T), alignof(T));
        inner_.deallocate(p, n);
    }

    const Inner& inner() const {
        return inner_;
    }

private:
    Inner inner_;
};

template<typename T, typename U, typename UpstreamT, typename UpstreamU>
bool operator==(const DebugAllocator<T, UpstreamT>& a, const DebugAllocator<U, UpstreamU>& b) {
    return a.inner() == b.inner();
}

template<typename T, typename U, typename UpstreamT, typename UpstreamU>
bool operator!=(const DebugAllocator<T, UpstreamT>& a, const DebugAllocator<U, UpstreamU>& b) {
    return a.inner() != b.inner();
}
