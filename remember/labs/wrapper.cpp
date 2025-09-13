#include "../src/include/lib.hpp"
#include <cstdio>
#include <ranges>
#include <vector>

int main() {
  ArenaAllocator<int> alloc;
  {
    std::vector<int, ArenaAllocator<int>> vec(alloc);

    for (auto i : std::ranges::views::iota(0, 64)) {
      vec.push_back(i);
    }

    for (auto i : vec) {
      printf("%i ", i);
    }
  }

  // AllocationTracker::instance().print_summary();

  return 0;
}
