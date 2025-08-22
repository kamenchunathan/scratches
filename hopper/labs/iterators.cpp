#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iterator>
#include <ranges>
#include <string>
#include <tuple>
#include <vector>

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
             const std::function<std::uint32_t()> op)
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
    const std::function<std::uint32_t()> op_;
  };

  Iterator begin() { return Iterator(0, count_, expenny); };
  Iterator end() { return Iterator(count_, count_, expenny); };

private:
  std::uint32_t count_;
  static std::uint32_t expenny() {
    std::printf("Expensive operation\n");
    return 30;
  }
};

struct MyData {
  int id;
  std::string name;
  double value;
};

class SubsetIterator {
public:
  struct Iterator {
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::tuple<int, std::string>;
    using reference = std::tuple<int &, std::string &>;
    using pointer = void;

    Iterator() = default;
    explicit Iterator(std::vector<MyData>::iterator it) : it_(it) {}

    reference operator*() const { return std::tie(it_->id, it_->name); }

    Iterator &operator++() {
      ++it_;
      return *this;
    }
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const Iterator &a, const Iterator &b) {
      return a.it_ == b.it_;
    }
    friend bool operator!=(const Iterator &a, const Iterator &b) {
      return a.it_ != b.it_;
    }

  private:
    std::vector<MyData>::iterator it_{};
  };

  SubsetIterator() = default;
  explicit SubsetIterator(std::vector<MyData> &data) : vec_(&data) {}

  Iterator begin() { return Iterator(vec_->begin()); }
  Iterator end() { return Iterator(vec_->end()); }

  std::size_t size() const noexcept { return vec_ ? vec_->size() : 0; }

private:
  std::vector<MyData> *vec_ = nullptr;
};

int main() {
  Sequence infinite_numbers(1);
  for (auto v : infinite_numbers | std::ranges::views::take(5)) {
    printf("%u ", v);
  }

  for (auto v : Expenny(4)) {
    printf("bingo %u ", v);
  }
  printf("\n");

  std::vector<MyData> items = {
      {1, "Alice", 10.5}, {2, "Bob", 20.5}, {3, "Charlie", 30.5}};

  // Works as an lvalue range with views::take
  SubsetIterator subset(items);
  for (auto [id, name] : subset | std::views::take(2)) {
    std::printf("Before edit: id=%d name=%s\n", id, name.c_str());

    // Modify via references in the tuple
    id += 100;
    name += "_updated";

    std::printf("After edit: id=%d name=%s\n", id, name.c_str());
  }

  std::puts("--- Original data after edits ---");
  for (const auto &d : items) {
    std::printf("id=%d name=%s value=%.1f\n", d.id, d.name.c_str(), d.value);
  }

  std::puts("--- Using as a temporary with views::drop ---");
  for (auto [id, name] : SubsetIterator(items) | std::views::drop(1)) {
    std::printf("id=%d name=%s\n", id, name.c_str());
  }
}
