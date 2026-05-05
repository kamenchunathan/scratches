#pragma once

#include <cstdint>
#include <functional>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ecs/type_registry.hpp"
#include "ecs/world.hpp"

namespace ecs {

enum class Stage : std::uint8_t {
    Startup,
    PreUpdate,
    Update,
    PostUpdate,
    FixedUpdate,
    Count // Sentinel for array sizing
};


struct SystemSetTag {};
using SystemSetRegistry = TypeRegistry<SystemSetTag>;

class Schedule;

template<typename Set>
class SetBuilder {
public:
    SetBuilder(Schedule& schedule, uint32_t set_id) 
        : schedule_(schedule), set_id_(set_id) {}

    template<typename Other>
    SetBuilder& before();

    template<typename Other>
    SetBuilder& after();

private:
    Schedule& schedule_;
    uint32_t set_id_;
};

class SystemBuilder {
public:
    SystemBuilder(Schedule& schedule, std::size_t sys_index) 
        : schedule_(schedule), sys_index_(sys_index) {}

    template<typename Set>
    SystemBuilder& in_set();

    template<typename Set>
    SystemBuilder& before();

    template<typename Set>
    SystemBuilder& after();

private:
    Schedule& schedule_;
    std::size_t sys_index_;
};

using System = std::function<void(World&)>;

class Schedule {
public:
    inline static constexpr uint32_t DEFAULT_SET = 0xFFFFFFFF;

    SystemBuilder add_system(System sys) {
        std::size_t index = systems_.size();
        systems_.push_back({std::move(sys), DEFAULT_SET});
        dirty_ = true;
        return SystemBuilder(*this, index);
    }

    template<typename Set>
    SetBuilder<Set> configure_set() {
        uint32_t set_id = SystemSetRegistry::get_id<Set>();
        sets_[set_id] = {}; 
        dirty_ = true;
        return SetBuilder<Set>(*this, set_id);
    }

    void build() {
        if (!dirty_) return;

        std::unordered_map<uint32_t, std::vector<uint32_t>> adj;
        std::unordered_map<uint32_t, uint32_t> in_degree;

        // 1. Initialize in-degrees for all known sets, implicit sets, and DEFAULT_SET
        in_degree[DEFAULT_SET] = 0;
        for (const auto& [set_id, entry] : sets_) {
            in_degree[set_id] = 0;
        }
        for (const auto& sys : systems_) {
            in_degree[sys.set_id] = 0;
        }

        // 2. Build graph edges
        for (const auto& [set_id, entry] : sets_) {
            for (uint32_t b : entry.before) {
                adj[set_id].push_back(b);
                in_degree[b]++;
            }
            for (uint32_t a : entry.after) {
                adj[a].push_back(set_id);
                in_degree[set_id]++;
            }
            
            // Default rule: All named/implicit sets run before the DEFAULT_SET 
            // unless explicit constraints dictate otherwise.
            if (set_id != DEFAULT_SET) {
                adj[set_id].push_back(DEFAULT_SET);
                in_degree[DEFAULT_SET]++;
            }
        }

        // 3. Kahn's Algorithm for Topological Sort
        std::queue<uint32_t> q;
        for (const auto& [node, deg] : in_degree) {
            if (deg == 0) q.push(node);
        }

        std::vector<uint32_t> sorted_sets;
        while (!q.empty()) {
            uint32_t curr = q.front();
            q.pop();
            sorted_sets.push_back(curr);

            for (uint32_t neighbor : adj[curr]) {
                if (--in_degree[neighbor] == 0) {
                    q.push(neighbor);
                }
            }
        }

        if (sorted_sets.size() != in_degree.size()) {
            throw std::runtime_error("Schedule Build Error: Circular dependency detected in system sets.");
        }

        // 4. Group systems into their sorted buckets (preserving registration order internally)
        std::unordered_map<uint32_t, std::vector<std::size_t>> bucketed_systems;
        for (std::size_t i = 0; i < systems_.size(); ++i) {
            bucketed_systems[systems_[i].set_id].push_back(i);
        }

        sorted_indices_.clear();
        for (uint32_t set_id : sorted_sets) {
            for (std::size_t sys_idx : bucketed_systems[set_id]) {
                sorted_indices_.push_back(sys_idx);
            }
        }

        dirty_ = false;
    }

    void run(World& world) {
        if (dirty_) build();
        
        for (std::size_t idx : sorted_indices_) {
            systems_[idx].callable(world);
        }
    }

    // Internal API for Builders
    void add_set_edge_before(uint32_t set_id, uint32_t target) {
        sets_[set_id].before.push_back(target);
        dirty_ = true;
    }

    void add_set_edge_after(uint32_t set_id, uint32_t target) {
        sets_[set_id].after.push_back(target);
        dirty_ = true;
    }

    void assign_system_to_set(std::size_t sys_index, uint32_t set_id) {
        systems_[sys_index].set_id = set_id;
        dirty_ = true;
    }

    uint32_t get_or_create_implicit_set(std::size_t sys_index) {
        if (systems_[sys_index].set_id == DEFAULT_SET) {
            systems_[sys_index].set_id = next_implicit_set_++;
        }
        return systems_[sys_index].set_id;
    }

private:
    struct SystemEntry {
        System callable;
        uint32_t set_id;
    };

    struct SetEntry {
        std::vector<uint32_t> before;
        std::vector<uint32_t> after;
    };

    std::vector<SystemEntry> systems_;
    std::unordered_map<uint32_t, SetEntry> sets_;
    std::vector<std::size_t> sorted_indices_;
    
    bool dirty_ = false;
    uint32_t next_implicit_set_ = 0x80000000; // Keep implicit sets isolated
};


// Builder Implementations 

template<typename Set>
template<typename Other>
SetBuilder<Set>& SetBuilder<Set>::before() {
    schedule_.add_set_edge_before(set_id_, SystemSetRegistry::get_id<Other>());
    return *this;
}

template<typename Set>
template<typename Other>
SetBuilder<Set>& SetBuilder<Set>::after() {
    schedule_.add_set_edge_after(set_id_, SystemSetRegistry::get_id<Other>());
    return *this;
}

template<typename Set>
SystemBuilder& SystemBuilder::in_set() {
    schedule_.assign_system_to_set(sys_index_, SystemSetRegistry::get_id<Set>());
    return *this;
}

template<typename Set>
SystemBuilder& SystemBuilder::before() {
    uint32_t implicit_set = schedule_.get_or_create_implicit_set(sys_index_);
    schedule_.add_set_edge_before(implicit_set, SystemSetRegistry::get_id<Set>());
    return *this;
}

template<typename Set>
SystemBuilder& SystemBuilder::after() {
    uint32_t implicit_set = schedule_.get_or_create_implicit_set(sys_index_);
    schedule_.add_set_edge_after(implicit_set, SystemSetRegistry::get_id<Set>());
    return *this;
}

class Scheduler {
public:
    SystemBuilder add_system(Stage stage, System&& sys) {
        return schedules_[static_cast<std::size_t>(stage)].add_system(std::move(sys));
    }

    template<typename Set>
    SetBuilder<Set> configure_set(Stage stage) {
        return schedules_[static_cast<std::size_t>(stage)].configure_set<Set>();
    }

    void run_stage(Stage stage, World& world) {
        schedules_[static_cast<std::size_t>(stage)].run(world);
    }

private:
    std::array<Schedule, static_cast<std::size_t>(Stage::Count)> schedules_;
};

} // namespace ecs
