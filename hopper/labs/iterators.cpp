#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
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

// An sequence of expensively calulated numbers to simulate an
// expensive operation and designing an iterator to only compute expensive
// function calls when necessary
class Expenny {
public:
  Expenny(std::uint32_t c) : count_(c) {};

  class Iterator {
  public:
    Iterator(std::uint32_t start, std::uint32_t count,
             const std::function<std::uint32_t()> &op)
        : current_(start), count_(count), op_(op) {};

    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = const uint32_t;
    using pointer = const uint32_t *;
    using reference = const uint32_t &;

    reference operator*() {
      computed_ = op_();
      return computed_;
    };
    pointer operator->() { return &computed_; }

    Iterator &operator++() {
      ++current_;
      return *this;
    };
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    };

    friend bool operator==(const Iterator &a, const Iterator &b) {
      return a.current_ == b.current_;
    }

    friend bool operator!=(const Iterator &a, const Iterator &b) {
      return a.current_ != b.current_;
    }

  private:
    std::uint32_t current_, count_, computed_;
    const std::function<std::uint32_t()> &op_;
  };

  Iterator begin() { return Iterator(0, count_, expenny); };
  Iterator end() { return Iterator(count_, count_, expenny); };

private:
  std::uint32_t count_;
  static std::uint32_t expenny() {
    std::printf("Expensive operation");
    return 30;
  }
};

int main() {
  Sequence infinite_numbers(1);
  for (auto v : infinite_numbers | std::ranges::views::take(5)) {
    printf("%u ", v);
  }

  for (auto v : Expenny(2)) {
    printf("bingo %u ", v);
  }
  printf("\n");
  return 0;
}
