#include "renderer/graph.hpp"

#include <stdexcept>
#include <utility>

namespace renderer {

RenderPass::RenderPass(std::string name): name_(std::move(name)) {}

void RenderPass::add_dependency(const std::string& pass_name) {
    dependencies_.push_back(pass_name);
}

const std::string& RenderPass::name() const {
    return name_;
}

const std::vector<std::string>& RenderPass::dependencies() const {
    return dependencies_;
}

void RenderGraph::add_pass(std::unique_ptr<RenderPass> pass) {
    pass_map_[pass->name()] = pass.get();
    passes_.push_back(std::move(pass));
    compiled_ = false;
}

const std::vector<RenderPass*>& RenderGraph::compile() {
    if (!compiled_) {
        topological_sort();
        compiled_ = true;
    }
    return compiled_order_;
}

RenderPass* RenderGraph::get_pass(const std::string& name) const {
    auto it = pass_map_.find(name);
    if (it != pass_map_.end()) {
        return it->second;
    }
    return nullptr;
}

void RenderGraph::topological_sort() {
    compiled_order_.clear();
    std::unordered_map<std::string, int> in_degree;
    std::unordered_map<std::string, std::vector<std::string>> adj;

    for (const auto& pass: passes_) {
        in_degree[pass->name()] = 0;
    }

    for (const auto& pass: passes_) {
        for (const auto& dep_name: pass->dependencies()) {
            if (pass_map_.find(dep_name) == pass_map_.end()) {
                throw std::runtime_error(
                    "Dependency '" + dep_name + "' for pass '" + pass->name() + "' not found."
                );
            }
            adj[dep_name].push_back(pass->name());
            in_degree[pass->name()]++;
        }
    }

    std::vector<std::string> queue;
    for (const auto& pass: passes_) {
        if (in_degree[pass->name()] == 0) {
            queue.push_back(pass->name());
        }
    }

    while (!queue.empty()) {
        std::string u_name = queue.back();
        queue.pop_back();
        compiled_order_.push_back(pass_map_.at(u_name));

        if (adj.count(u_name)) {
            for (const auto& v_name: adj.at(u_name)) {
                in_degree[v_name]--;
                if (in_degree[v_name] == 0) {
                    queue.push_back(v_name);
                }
            }
        }
    }

    if (compiled_order_.size() != passes_.size()) {
        compiled_order_.clear();
        throw std::runtime_error("Cycle detected in render graph.");
    }
}

} // namespace renderer
