#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace renderer {

class RenderPass {
public:
    explicit RenderPass(std::string name);
    virtual ~RenderPass() = default;

    void add_dependency(const std::string& pass_name);

    const std::string& name() const;
    const std::vector<std::string>& dependencies() const;

protected:
    std::string name_;
    std::vector<std::string> dependencies_;
};

class RenderGraph {
public:
    void add_pass(std::unique_ptr<RenderPass> pass);
    const std::vector<RenderPass*>& compile();
    RenderPass* get_pass(const std::string& name) const;

private:
    void topological_sort();

    std::vector<std::unique_ptr<RenderPass>> passes_;
    std::unordered_map<std::string, RenderPass*> pass_map_;
    std::vector<RenderPass*> compiled_order_;
    bool compiled_ = false;
};

} // namespace renderer
