#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <ranges>

/// An iterator that continuously returns the same value
/// Following this tutorial
/// https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp
class Sequence {
public:
  struct Iterator {

    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = uint32_t;
    using pointer = uint32_t *;
    using reference = uint32_t &;

    Iterator(uint32_t v) : value_(v) {};
    Iterator() : value_(0) {};

    reference operator*() { return value_; };
    pointer operator->() { return &value_; };

    Iterator &operator++() {
      ++value_;
      return *this;
    };
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    };

    friend bool operator==(const Iterator &a, const Iterator &b) {
      return a.value_ == b.value_;
    }

    friend bool operator!=(const Iterator &a, const Iterator &b) {
      return a.value_ != b.value_;
    }

  private:
    std::uint32_t value_;
  };

  Iterator begin() { return Iterator(start_value_); }
  Iterator end() { return Iterator(); }

  Sequence(uint32_t start) : start_value_(start) {}

private:
  uint32_t start_value_;
};

int main() {
  Sequence infinite_numbers(1);
  for (auto v : infinite_numbers | std::ranges::views::take(5)) {
    printf("%u ", v);
  }
  printf("\n");
  return 0;
}
