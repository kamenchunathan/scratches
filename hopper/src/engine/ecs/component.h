
#pragma once

#include <atomic>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <functional>

const std::uint32_t MAX_COMPONENTS = 64;

using ComponentMask = std::bitset<MAX_COMPONENTS>;

// NOTE: The signature seems to be different when a type is passed in as an
// lvalue and rvalue. I don't know why this is but it probbly results from hving
// different component ids. Why? idk to fix later
struct Signature {
  std::bitset<MAX_COMPONENTS> mask;

  bool operator==(const Signature &other) const { return mask == other.mask; }
};

struct SignatureHash {
  std::size_t operator()(const Signature &s) const {
    return std::hash<std::bitset<MAX_COMPONENTS>>{}(s.mask);
  }
};

using ComponentId = std::uint32_t;

class ComponentIds {
  static inline std::atomic<uint32_t> counter = 0;

public:
  template <typename T> static std::size_t get_id() {
    static std::uint32_t id = counter.fetch_add(1, std::memory_order_seq_cst);
    return id;
  }
};
