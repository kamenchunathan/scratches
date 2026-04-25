#pragma once

#include <concepts>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include <Eigen/Dense>

#include "version.hpp"

namespace renderer {

enum class Primitive { Lines, Triangles, Square };

struct PipelineDescriptor {};

/* Provides a way to count the number of fields */
namespace detail {

    struct any_type {
        template<typename T>
        operator T();
    };

    template<typename T, typename Indices>
    struct can_aggregate_init;

    template<typename T, std::size_t... Is>
    struct can_aggregate_init<T, std::index_sequence<Is...>> {
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
            if constexpr (can_aggregate_init<T, std::make_index_sequence<Mid>>::value) {
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

    template<typename T>
    auto tuple_from_aggregate(T&& obj) {
        constexpr std::size_t n = detail::count_fields<std::remove_cvref_t<T>>();
        if constexpr (n == 0) {
            return std::tuple {};
        } else if constexpr (n == 1) {
            auto&& [a] = std::forward<T>(obj);
            return std::tuple {std::forward<decltype(a)>(a)};
        } else if constexpr (n == 2) {
            auto&& [a, b] = std::forward<T>(obj);
            return std::tuple {std::forward<decltype(a)>(a), std::forward<decltype(b)>(b)};
        } else if constexpr (n == 3) {
            auto&& [a, b, c] = std::forward<T>(obj);
            return std::tuple {
                std::forward<decltype(a)>(a),
                std::forward<decltype(b)>(b),
                std::forward<decltype(c)>(c)
            };
        } else if constexpr (n == 4) {
            auto&& [a, b, c, d] = std::forward<T>(obj);
            return std::tuple {
                std::forward<decltype(a)>(a),
                std::forward<decltype(b)>(b),
                std::forward<decltype(c)>(c),
                std::forward<decltype(d)>(d)
            };
        } else if constexpr (n == 5) {
            auto&& [a, b, c, d, e] = std::forward<T>(obj);
            return std::tuple {
                std::forward<decltype(a)>(a),
                std::forward<decltype(b)>(b),
                std::forward<decltype(c)>(c),
                std::forward<decltype(d)>(d),
                std::forward<decltype(e)>(e)
            };
        } else if constexpr (n == 6) {
            auto&& [a, b, c, d, e, f] = std::forward<T>(obj);
            return std::tuple {
                std::forward<decltype(a)>(a),
                std::forward<decltype(b)>(b),
                std::forward<decltype(c)>(c),
                std::forward<decltype(d)>(d),
                std::forward<decltype(e)>(e),
                std::forward<decltype(f)>(f)
            };
        } else if constexpr (n == 7) {
            auto&& [a, b, c, d, e, f, g] = std::forward<T>(obj);
            return std::tuple {
                std::forward<decltype(a)>(a),
                std::forward<decltype(b)>(b),
                std::forward<decltype(c)>(c),
                std::forward<decltype(d)>(d),
                std::forward<decltype(e)>(e),
                std::forward<decltype(f)>(f),
                std::forward<decltype(g)>(g)
            };
        } else if constexpr (n == 8) {
            auto&& [a, b, c, d, e, f, g, h] = std::forward<T>(obj);
            return std::tuple {
                std::forward<decltype(a)>(a),
                std::forward<decltype(b)>(b),
                std::forward<decltype(c)>(c),
                std::forward<decltype(d)>(d),
                std::forward<decltype(e)>(e),
                std::forward<decltype(f)>(f),
                std::forward<decltype(g)>(g),
                std::forward<decltype(h)>(h)
            };
        } else if constexpr (n == 9) {
            auto&& [a, b, c, d, e, f, g, h, i] = std::forward<T>(obj);
            return std::tuple {
                std::forward<decltype(a)>(a),
                std::forward<decltype(b)>(b),
                std::forward<decltype(c)>(c),
                std::forward<decltype(d)>(d),
                std::forward<decltype(e)>(e),
                std::forward<decltype(f)>(f),
                std::forward<decltype(g)>(g),
                std::forward<decltype(h)>(h),
                std::forward<decltype(i)>(i)
            };
        } else if constexpr (n == 10) {
            auto&& [a, b, c, d, e, f, g, h, i, j] = std::forward<T>(obj);
            return std::tuple {
                std::forward<decltype(a)>(a),
                std::forward<decltype(b)>(b),
                std::forward<decltype(c)>(c),
                std::forward<decltype(d)>(d),
                std::forward<decltype(e)>(e),
                std::forward<decltype(f)>(f),
                std::forward<decltype(g)>(g),
                std::forward<decltype(h)>(h),
                std::forward<decltype(i)>(i),
                std::forward<decltype(j)>(j)
            };
        } else {
            static_assert(n <= 10, "Aggregates with more than 10 fields are not supported");
            return std::tuple {};
        }
    }

    template<typename T, typename... Args, std::size_t... Is>
    T construct_from_tuple_impl(std::tuple<Args...>&& t, std::index_sequence<Is...>) {
        return T {std::get<Is>(std::move(t))...};
    }

    template<typename T, typename... Args>
    T construct_from_tuple(std::tuple<Args...>&& t) {
        return construct_from_tuple_impl<T>(
            std::move(t),
            std::make_index_sequence<sizeof...(Args)> {}
        );
    }
} // namespace detail

template<typename T>
concept Interpolatable = requires(T a, T b, float t) {
    {
        a + b
    } -> std::convertible_to<T>;

    {
        a* t
    } -> std::convertible_to<T>;
};

template<typename T>
concept AggregateInterpolatable = std::is_aggregate_v<T> && requires {
    []<typename... Fields>(std::tuple<Fields...>*) {
        return (Interpolatable<std::remove_cvref_t<Fields>> && ...);
    }(static_cast<decltype(detail::tuple_from_aggregate(std::declval<T>()))*>(nullptr));
};

template<typename T>
concept HasPosition = requires(T t) {
    {
        t.position
    } -> std::convertible_to<Eigen::Vector4f>;
};

template<typename VertexIn, typename VertexOut, typename FragOut, typename... Uniforms>
    requires HasPosition<VertexOut> && AggregateInterpolatable<VertexOut>
class Shader {
public:
    virtual ~Shader() = default;
    virtual VertexOut vertex(const VertexIn&) = 0;
    virtual FragOut fragment(const VertexOut&) = 0;

    template<std::uint32_t id, typename T>
    void bind_uniform(T t) {
        std::get<id>(uniforms_) = t;
    }

private:
    std::tuple<std::optional<Uniforms>...> uniforms_;
};

template<typename VertexIn, typename VertexOut, typename FragOut, typename... Uniforms>
    requires HasPosition<VertexOut> && AggregateInterpolatable<VertexOut>
class HOPPER_DEPRECATED(0, 2, "renderer::Shader") ShaderPrev {
public:
    virtual ~ShaderPrev() = default;
    virtual VertexOut vertex(const VertexIn&, const Uniforms&...) = 0;
    virtual FragOut fragment(const VertexOut&, const Uniforms&...) = 0;
};

class IPipeline {
public:
    virtual ~IPipeline() = default;
};

template<typename VertexIn, typename VertexOut, typename FragOut, typename... Uniforms>
class Pipeline: public IPipeline {
public:
    using vertex_in = VertexIn;
    using vertex_out = VertexOut;
    using frag_out = FragOut;
    using uniforms = std::tuple<Uniforms...>;

    Pipeline(
        PipelineDescriptor,
        std::unique_ptr<ShaderPrev<VertexIn, VertexOut, FragOut, Uniforms...>> shader
    ):
        shader(std::move(shader)) {}

    std::unique_ptr<ShaderPrev<VertexIn, VertexOut, FragOut, Uniforms...>> shader;

private:
    PipelineDescriptor descriptor_;
};

template<typename VertexIn, typename VertexOut, typename FragOut, typename... Uniforms>
class HOPPER_DEPRECATED(0, 2, "renderer::Pipeline") PipelinePrev: public IPipeline {
public:
    PipelinePrev(
        PipelineDescriptor,
        std::unique_ptr<ShaderPrev<VertexIn, VertexOut, FragOut, Uniforms...>> shader
    ):
        shader(std::move(shader)) {}
    std::unique_ptr<ShaderPrev<VertexIn, VertexOut, FragOut, Uniforms...>> shader;

private:
    PipelineDescriptor descriptor_;
};

class PipelineRegistry {
public:
    template<typename Pipeline>
    void register_pipeline(std::unique_ptr<Pipeline> pipeline) {
        pipelines_.insert_or_assign(std::type_index(typeid(Pipeline)), std::move(pipeline));
    }

    template<typename T>
    T* get_pipeline() {
        auto it = pipelines_.find(std::type_index(typeid(T)));
        if (it == pipelines_.end())
            return nullptr;

        return dynamic_cast<T*>(it->second.get());
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<IPipeline>> pipelines_;
};

} // namespace renderer
