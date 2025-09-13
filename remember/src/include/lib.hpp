#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <new>

template <typename T> class WrapperAllocator {
public:
  using value_type = T;

  WrapperAllocator() = default;
  template <typename U>
  constexpr WrapperAllocator(const WrapperAllocator<U> &) noexcept {}

  T *allocate(std::size_t n) {
    return static_cast<T *>(::operator new(n * sizeof(T)));
  }

  void deallocate(T *p, std::size_t) noexcept { ::operator delete(p); }

  friend bool operator==(const WrapperAllocator &, const WrapperAllocator &) {
    return true;
  }

  friend bool operator!=(const WrapperAllocator &, const WrapperAllocator &) {
    return false;
  }
};

class AllocationTracker {
public:
  AllocationTracker() = default;

  static AllocationTracker &instance() {
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
    std::printf("Total allocations: %lu\n Totall memory allocated: %lu\n"
                " Total deallocations: %lu\n Totall memory deallocated: %lu\n",
                total_allocations_, total_allocated_, total_deallocations_,
                total_deallocated_);
  }

private:
  mutable std::mutex mutex_;
  size_t total_allocations_ = 0;
  size_t total_allocated_ = 0;
  size_t total_deallocations_ = 0;
  size_t total_deallocated_ = 0;
};

template <typename T> class DebugAllocator {
public:
  using value_type = T;

  DebugAllocator() = default;
  template <typename U>
  constexpr DebugAllocator(const DebugAllocator<U> &) noexcept {}

  T *allocate(std::size_t n) {
    AllocationTracker::instance().record_allocation(n * sizeof(T), alignof(T));
    return static_cast<T *>(::operator new(n * sizeof(T)));
  }

  void deallocate(T *p, std::size_t n) noexcept {
    AllocationTracker::instance().record_deallocation(n * sizeof(T),
                                                      alignof(T));
    ::operator delete(p);
  }

  friend bool operator==(const DebugAllocator &, const DebugAllocator &) {
    return true;
  }

  friend bool operator!=(const DebugAllocator &, const DebugAllocator &) {
    return false;
  }
};

template <typename T> class Mallocator {
public:
  using value_type = T;

  Mallocator() noexcept = default;

  T *allocate(std::size_t n) {
    if (n == 0) {
      return nullptr;
    }

    if (n * sizeof(T) > std::numeric_limits<std::size_t>::max()) {
      throw std::bad_array_new_length();
    }

    void *ptr = malloc(n * sizeof(T));
    if (ptr == nullptr) {
      throw std::bad_alloc();
    }

    return static_cast<T *>(ptr);
  }

  void deallocate(T *p, std::size_t) { free(p); }
};

template <typename T> class ArenaAllocator {
public:
  ArenaAllocator() noexcept = default;

  ArenaAllocator(std::size_t alloc_size) noexcept : alloc_size_(alloc_size) {}

  T *allocate(std::size_t n) {}

private:
  void *alloc_ = nullptr;
  std::size_t offset_ = 0;
  std::size_t alloc_size_ = 1024;
};
