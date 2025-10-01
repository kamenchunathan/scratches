#pragma once

#include <any>
#include <memory>
#include <typeindex>
#include <unordered_map>

#include "renderer/resource.hpp"

namespace renderer {

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
    Pipeline(std::unique_ptr<Shader<VertexIn, VertexOut, FragOut, RequiredResources...>> shader):
        shader_(std::move(shader)) {}

private:
    std::unique_ptr<Shader<VertexIn, VertexOut, FragOut, RequiredResources...>> shader_;
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

        try {
            return std::any_cast<std::unique_ptr<T>>(&it->second)->get();
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<IPipeline>> pipelines_;
};

} // namespace renderer
