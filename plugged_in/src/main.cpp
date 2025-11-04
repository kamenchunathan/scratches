#include <print>

namespace detail {};

template <typename T>
concept Sortable = requires(T a, T b) {
  { a < b } -> std::convertible_to<bool>;
};

int main() { std::println("Bingo"); }
