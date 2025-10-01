#include <any>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>

namespace renderer {

template<typename... Resources>
struct ResourcePack {
    std::tuple<Resources...> resources;

    template<typename T>
    T& get() {
        return std::get<T>(resources);
    };

    template<typename T>
    const T& get() const {
        return std::get<T>(resources);
    };

    template<typename NewResource>
    auto add(NewResource&& resource) const {
        return ResourcePack<Resources..., std::decay_t<NewResource>> {
            std::tuple_cat(resources, std::make_tuple(std::forward<NewResource>(resource)))
        };
    }
};

template<typename VertexIn, typename VertexOut, typename FragOut, typename... RequiredResources>
class Shader {
public:
    virtual ~Shader() = default;
    virtual VertexOut vertex(const VertexIn&, const ResourcePack<RequiredResources...>&) = 0;
    virtual FragOut fragment(const VertexOut&, const ResourcePack<RequiredResources...>&) = 0;
};

class IPipeline {
public:
    virtual ~IPipeline() = default;
};

template<typename VertexIn, typename VertexOut, typename FragOut, typename... RequiredResources>
class Pipeline: public IPipeline {
public:
    Pipeline(
        std::unique_ptr<Shader<VertexIn, VertexOut, FragOut, RequiredResources...>> shader,
        std::string name
    ):
        shader_(std::move(shader)),
        name_(name) {}

    const std::string& name() const {
        return name_;
    }

private:
    std::unique_ptr<Shader<VertexIn, VertexOut, FragOut, RequiredResources...>> shader_;
    std::string name_;
};

class PipelineRegistry {
public:
    template<typename T>
    void register_pipeline(const std::string& name, std::unique_ptr<T> pipeline) {
        pipelines_.try_emplace(name, std::move(pipeline));
    }

    template<typename T>
    T* get_pipeline(const std::string& name) {
        auto it = pipelines_.find(name);
        if (it == pipelines_.end())
            return nullptr;

        try {
            return std::any_cast<std::unique_ptr<T>>(&it->second)->get();
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }

private:
    std::unordered_map<std::string, std::any> pipelines_;
};

} // namespace renderer
