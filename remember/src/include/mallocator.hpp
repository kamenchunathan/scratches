#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>

/**
 * @brief A stateless, fundamental allocator that uses malloc() and free().
 */
template<typename T>
class Mallocator {
public:
    using value_type = T;

    Mallocator() noexcept = default;
    template<typename U>
    constexpr Mallocator(const Mallocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }
        if (auto p = static_cast<T*>(std::malloc(n * sizeof(T)))) {
            return p;
        }
        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t) noexcept {
        std::free(p);
    }
};

template<typename T, typename U>
bool operator==(const Mallocator<T>&, const Mallocator<U>&) {
    return true;
}

template<typename T, typename U>
bool operator!=(const Mallocator<T>&, const Mallocator<U>&) {
    return false;
}
