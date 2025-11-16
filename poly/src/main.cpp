#include <cstdint>
#include <meta>
#include <print>

struct Point {
    std::uint32_t x, y;
};

int main() {
    constexpr auto point_refl = ^^Point;
    constexpr auto type_name = std::meta::is_namespace_member(point_refl);
    std::println("type name {}", type_name);
    return 0;
}
