// Copyright 2026 Nova ROS, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef LAUNCH_CPP_DEPENDENCY_RESOLVER_HPP_
#define LAUNCH_CPP_DEPENDENCY_RESOLVER_HPP_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <stdexcept>

namespace launch_cpp {

/**
 * @brief Node configuration for dependency resolution
 */
struct NodeConfig {
    std::string name;
    std::vector<std::string> dependencies;

    bool operator==(const NodeConfig& other) const {
        return name == other.name;
    }

    bool operator<(const NodeConfig& other) const {
        return name < other.name;
    }
};

/**
 * @brief Result of dependency resolution
 */
struct ResolutionResult {
    bool success{true};
    std::vector<std::string> order;
    std::string error_message;
    std::vector<std::string> circular_path;
};

/**
 * @brief Dependency resolver using Kahn's algorithm
 *
 * This class implements topological sorting to determine
 * the correct startup order based on node dependencies.
 *
 * Algorithm: Kahn's Algorithm
 * Time Complexity: O(V + E)
 * Space Complexity: O(V)
 *
 * Where V = number of nodes, E = number of dependencies
 */
class DependencyResolver {
public:
    DependencyResolver() = default;
    ~DependencyResolver() = default;

    // Non-copyable
    DependencyResolver(const DependencyResolver&) = delete;
    DependencyResolver& operator=(const DependencyResolver&) = delete;

    // Movable
    DependencyResolver(DependencyResolver&&) = default;
    DependencyResolver& operator=(DependencyResolver&&) = default;

    /**
     * @brief Resolve dependencies and return startup order
     * @param nodes Vector of node configurations
     * @return ResolutionResult containing order or error
     *
     * @requirement TSR-002: Startup sequence control
     *
     * Example:
     * @code
     * std::vector<NodeConfig> nodes = {
     *     {"A", {}},           // No dependencies
     *     {"B", {"A"}},        // Depends on A
     *     {"C", {"A", "B"}}    // Depends on A and B
     * };
     * auto result = resolver.resolve(nodes);
     * // result.order = {"A", "B", "C"}
     * @endcode
     */
    ResolutionResult resolve(const std::vector<NodeConfig>& nodes);

    /**
     * @brief Check if dependency graph has circular dependencies
     * @param nodes Vector of node configurations
     * @return true if circular dependency exists
     *
     * @requirement TSR-001.3: Dependency validation
     */
    bool has_circular_dependency(const std::vector<NodeConfig>& nodes);

    /**
     * @brief Get the circular path if one exists
     * @param nodes Vector of node configurations
     * @return Vector showing the circular path, empty if no cycle
     */
    std::vector<std::string> get_circular_path(const std::vector<NodeConfig>& nodes);

    /**
     * @brief Validate that all dependencies exist
     * @param nodes Vector of node configurations
     * @return true if all dependencies are valid
     *
     * Checks that every dependency references an existing node.
     */
    bool validate_dependencies(const std::vector<NodeConfig>& nodes);

    /**
     * @brief Get list of missing dependencies
     * @param nodes Vector of node configurations
     * @return Vector of missing dependency names
     */
    std::vector<std::string> get_missing_dependencies(const std::vector<NodeConfig>& nodes);

    /**
     * @brief Check if a node can be started (all dependencies satisfied)
     * @param node_name Name of the node to check
     * @param started_nodes Set of already started nodes
     * @param all_nodes Map of all node configurations
     * @return true if node can be started
     */
    bool can_start_node(
        const std::string& node_name,
        const std::set<std::string>& started_nodes,
        const std::map<std::string, NodeConfig>& all_nodes) const;

    /**
     * @brief Get nodes that can be started in parallel
     * @param remaining_nodes Set of nodes not yet started
     * @param started_nodes Set of already started nodes
     * @param all_nodes Map of all node configurations
     * @return Vector of node names ready to start
     *
     * @requirement TSR-002.3: Parallel startup
     */
    std::vector<std::string> get_ready_nodes(
        const std::set<std::string>& remaining_nodes,
        const std::set<std::string>& started_nodes,
        const std::map<std::string, NodeConfig>& all_nodes) const;

private:
    /**
     * @brief Build adjacency list from node configurations
     */
    std::map<std::string, std::vector<std::string>> build_adjacency_list(
        const std::vector<NodeConfig>& nodes) const;

    /**
     * @brief Build in-degree map from node configurations
     */
    std::map<std::string, int> build_in_degree_map(
        const std::vector<NodeConfig>& nodes) const;

    /**
     * @brief Detect cycle using DFS
     * @return Cycle path if found, empty otherwise
     */
    std::vector<std::string> detect_cycle_dfs(
        const std::map<std::string, std::vector<std::string>>& graph) const;

    /**
     * @brief DFS helper for cycle detection
     */
    bool dfs_visit(
        const std::string& node,
        const std::map<std::string, std::vector<std::string>>& graph,
        std::map<std::string, int>& color,
        std::vector<std::string>& path,
        std::vector<std::string>& cycle) const;

    /**
     * @brief Build node name to config map
     */
    std::map<std::string, NodeConfig> build_node_map(
        const std::vector<NodeConfig>& nodes) const;
};

// ============================================================================
// Inline Implementation
// ============================================================================

inline bool DependencyResolver::validate_dependencies(
    const std::vector<NodeConfig>& nodes) {
    return get_missing_dependencies(nodes).empty();
}

inline std::vector<std::string> DependencyResolver::get_missing_dependencies(
    const std::vector<NodeConfig>& nodes) {
    std::set<std::string> all_node_names;
    std::vector<std::string> missing;

    // Collect all node names
    for (const auto& node : nodes) {
        all_node_names.insert(node.name);
    }

    // Check each dependency
    for (const auto& node : nodes) {
        for (const auto& dep : node.dependencies) {
            if (all_node_names.find(dep) == all_node_names.end()) {
                missing.push_back(dep);
            }
        }
    }

    return missing;
}

inline std::map<std::string, NodeConfig> DependencyResolver::build_node_map(
    const std::vector<NodeConfig>& nodes) const {
    std::map<std::string, NodeConfig> node_map;
    for (const auto& node : nodes) {
        node_map[node.name] = node;
    }
    return node_map;
}

inline bool DependencyResolver::can_start_node(
    const std::string& node_name,
    const std::set<std::string>& started_nodes,
    const std::map<std::string, NodeConfig>& all_nodes) const {

    auto it = all_nodes.find(node_name);
    if (it == all_nodes.end()) {
        return false;
    }

    const auto& node = it->second;
    for (const auto& dep : node.dependencies) {
        if (started_nodes.find(dep) == started_nodes.end()) {
            return false;  // Dependency not started
        }
    }

    return true;
}

inline std::vector<std::string> DependencyResolver::get_ready_nodes(
    const std::set<std::string>& remaining_nodes,
    const std::set<std::string>& started_nodes,
    const std::map<std::string, NodeConfig>& all_nodes) const {

    std::vector<std::string> ready;

    for (const auto& node_name : remaining_nodes) {
        if (can_start_node(node_name, started_nodes, all_nodes)) {
            ready.push_back(node_name);
        }
    }

    return ready;
}

// ============================================================================
// Full Implementation
// ============================================================================

inline ResolutionResult DependencyResolver::resolve(
    const std::vector<NodeConfig>& nodes) {

    ResolutionResult result;

    // Step 1: Validate all dependencies exist
    auto missing = get_missing_dependencies(nodes);
    if (!missing.empty()) {
        result.success = false;
        result.error_message = "Missing dependencies: ";
        for (size_t i = 0; i < missing.size(); ++i) {
            if (i > 0) result.error_message += ", ";
            result.error_message += missing[i];
        }
        return result;
    }

    // Step 2: Check for circular dependencies
    if (has_circular_dependency(nodes)) {
        result.success = false;
        result.error_message = "Circular dependency detected";
        result.circular_path = get_circular_path(nodes);
        return result;
    }

    // Step 3: Build adjacency list and in-degree map
    auto graph = build_adjacency_list(nodes);
    auto in_degree = build_in_degree_map(nodes);

    // Step 4: Initialize queue with nodes having no dependencies
    std::queue<std::string> queue;
    for (const auto& node : nodes) {
        if (node.dependencies.empty()) {
            queue.push(node.name);
        }
    }

    // Step 5: Process nodes using Kahn's algorithm
    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();
        result.order.push_back(current);

        // Find all nodes that depend on current
        for (const auto& [node_name, deps] : graph) {
            if (std::find(deps.begin(), deps.end(), current) != deps.end()) {
                in_degree[node_name]--;
                if (in_degree[node_name] == 0) {
                    queue.push(node_name);
                }
            }
        }
    }

    // Step 6: Check if all nodes were processed
    if (result.order.size() != nodes.size()) {
        result.success = false;
        result.error_message = "Failed to resolve all dependencies";
        result.order.clear();
    }

    return result;
}

inline bool DependencyResolver::has_circular_dependency(
    const std::vector<NodeConfig>& nodes) {
    return !get_circular_path(nodes).empty();
}

inline std::vector<std::string> DependencyResolver::get_circular_path(
    const std::vector<NodeConfig>& nodes) {
    auto graph = build_adjacency_list(nodes);
    return detect_cycle_dfs(graph);
}

inline std::map<std::string, std::vector<std::string>>
DependencyResolver::build_adjacency_list(const std::vector<NodeConfig>& nodes) const {
    std::map<std::string, std::vector<std::string>> graph;

    for (const auto& node : nodes) {
        graph[node.name] = node.dependencies;
    }

    return graph;
}

inline std::map<std::string, int> DependencyResolver::build_in_degree_map(
    const std::vector<NodeConfig>& nodes) const {
    std::map<std::string, int> in_degree;

    // Initialize all nodes with 0
    for (const auto& node : nodes) {
        in_degree[node.name] = 0;
    }

    // Count incoming edges
    for (const auto& node : nodes) {
        for (const auto& dep : node.dependencies) {
            in_degree[node.name]++;
        }
    }

    return in_degree;
}

inline std::vector<std::string> DependencyResolver::detect_cycle_dfs(
    const std::map<std::string, std::vector<std::string>>& graph) const {

    std::map<std::string, int> color;  // 0 = white, 1 = gray, 2 = black
    std::vector<std::string> path;
    std::vector<std::string> cycle;

    // Initialize colors
    for (const auto& [node, _] : graph) {
        color[node] = 0;
    }

    // DFS from each unvisited node
    for (const auto& [node, _] : graph) {
        if (color[node] == 0) {
            if (dfs_visit(node, graph, color, path, cycle)) {
                return cycle;
            }
        }
    }

    return cycle;  // Empty if no cycle
}

inline bool DependencyResolver::dfs_visit(
    const std::string& node,
    const std::map<std::string, std::vector<std::string>>& graph,
    std::map<std::string, int>& color,
    std::vector<std::string>& path,
    std::vector<std::string>& cycle) const {

    color[node] = 1;  // Gray
    path.push_back(node);

    auto it = graph.find(node);
    if (it != graph.end()) {
        for (const auto& neighbor : it->second) {
            if (color[neighbor] == 1) {
                // Found back edge - cycle detected
                cycle.push_back(neighbor);
                for (auto it = path.rbegin(); it != path.rend(); ++it) {
                    cycle.push_back(*it);
                    if (*it == neighbor) break;
                }
                std::reverse(cycle.begin(), cycle.end());
                return true;
            }

            if (color[neighbor] == 0) {
                if (dfs_visit(neighbor, graph, color, path, cycle)) {
                    return true;
                }
            }
        }
    }

    color[node] = 2;  // Black
    path.pop_back();
    return false;
}

// ============================================================================
// Test Support
// ============================================================================

#ifdef BUILD_TESTING
/**
 * @brief Test fixture for DependencyResolver tests
 */
class DependencyResolverTestFixture {
public:
    DependencyResolver create_resolver() const {
        return DependencyResolver();
    }

    // Test case: Simple linear dependency A -> B -> C
    std::vector<NodeConfig> create_linear_dependencies() const {
        return {
            {"A", {}},
            {"B", {"A"}},
            {"C", {"B"}}
        };
    }

    // Test case: Diamond dependency A -> B, A -> C, B -> D, C -> D
    std::vector<NodeConfig> create_diamond_dependencies() const {
        return {
            {"A", {}},
            {"B", {"A"}},
            {"C", {"A"}},
            {"D", {"B", "C"}}
        };
    }

    // Test case: Circular dependency A -> B -> C -> A
    std::vector<NodeConfig> create_circular_dependencies() const {
        return {
            {"A", {"C"}},
            {"B", {"A"}},
            {"C", {"B"}}
        };
    }

    // Test case: Missing dependency
    std::vector<NodeConfig> create_missing_dependency() const {
        return {
            {"A", {}},
            {"B", {"C"}}  // C doesn't exist
        };
    }

    // Test case: Complex graph
    std::vector<NodeConfig> create_complex_dependencies() const {
        return {
            {"sensor", {}},
            {"filter", {"sensor"}},
            {"fusion", {"filter"}},
            {"planner", {"fusion"}},
            {"controller", {"planner"}},
            {"logger", {"sensor", "controller"}}
        };
    }
};
#endif  // BUILD_TESTING

} // namespace launch_cpp

#endif // CPP_LAUNCH_DEPENDENCY_RESOLVER_HPP_
