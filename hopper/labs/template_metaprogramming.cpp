/* An attempt to count the number of fields in a struct / aggregate type using template metaprogramming
 */

#include <utility>

struct any_type {
    template<typename T>
    operator T();
};

template<typename T, typename Indices>
struct has_n_fields;

template<typename T, std::size_t... Is>
struct has_n_fields<T, std::index_sequence<Is...>> {
    template<typename U>
    static auto test(int) -> decltype(U {((void)Is, any_type {})...}, std::true_type {});

    template<typename U>
    static auto test(...) -> std::false_type;

    static constexpr bool value = decltype(test<T>(0))::value;
};

template<typename T, std::size_t Low, std::size_t High>
consteval std::size_t count_fields_impl() {
    if constexpr (Low == High) {
        return Low;
    } else {
        constexpr std::size_t Mid = (Low + High + 1) / 2;
        if constexpr (has_n_fields<T, std::make_index_sequence<Mid>>::value) {
            return count_fields_impl<T, Mid, High>();
        } else {
            return count_fields_impl<T, Low, Mid - 1>();
        }
    }
}

template<typename T>
consteval std::size_t count_fields() {
    return count_fields_impl<T, 0, 10>();
}

struct Foo {};
static_assert(count_fields<Foo>() == 0);

struct Bar {};
static_assert(count_fields<Bar>() == 0);
