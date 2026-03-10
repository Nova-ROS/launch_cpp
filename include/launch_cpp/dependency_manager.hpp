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

/**
 * @file dependency_manager.hpp
 * @brief Manages process dependencies and startup order
 * 
 * @details Integrates DependencyResolver with LaunchService to ensure
 *          processes are started in the correct order based on their
 *          declared dependencies.
 * 
 * @ASIL ASIL B
 * 
 * @requirements
 * - REQ-LAUNCH-DEP-001: Resolve and respect process dependencies
 * - REQ-LAUNCH-DEP-002: Detect circular dependencies
 * - REQ-LAUNCH-DEP-003: Support parallel startup of independent processes
 */

#ifndef LAUNCH_CPP__DEPENDENCY_MANAGER_HPP_
#define LAUNCH_CPP__DEPENDENCY_MANAGER_HPP_

#include "launch_cpp/safety/dependency_resolver.hpp"
#include "launch_cpp/actions/execute_process.hpp"
#include "launch_cpp/launch_context.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>

namespace launch_cpp
{

/**
 * @brief Manages process dependencies and execution order
 * 
 * @details This class wraps DependencyResolver and provides a high-level
 *          interface for managing ExecuteProcess actions with dependencies.
 */
class DependencyManager
{
 public:
  DependencyManager() = default;
  ~DependencyManager() = default;

  // Non-copyable
  DependencyManager(const DependencyManager&) = delete;
  DependencyManager& operator=(const DependencyManager&) = delete;

  // Movable
  DependencyManager(DependencyManager&&) = default;
  DependencyManager& operator=(DependencyManager&&) = default;

  /**
   * @brief Add a process with its dependencies
   * @param name Process name
   * @param action ExecuteProcess action
   * @param dependencies List of process names this process depends on
   * @return Error if validation fails
   */
  Error AddProcess(
    const std::string& name,
    const std::shared_ptr<ExecuteProcess>& action,
    const std::vector<std::string>& dependencies);

  /**
   * @brief Resolve dependencies and return execution order
   * @return ResolutionResult containing ordered list or error
   * 
   * @requirement REQ-LAUNCH-DEP-001
   */
  ResolutionResult ResolveDependencies() const;

  /**
   * @brief Execute all processes in dependency order
   * @param context Launch context
   * @return Error if execution fails
   * 
   * @details Executes processes in topological order, waiting for
   *          dependencies to be ready before starting dependent processes.
   * 
   * @requirement REQ-LAUNCH-DEP-003
   */
  Error ExecuteAll(LaunchContext& context);

  /**
   * @brief Get a process by name
   * @param name Process name
   * @return Shared pointer to process, nullptr if not found
   */
  std::shared_ptr<ExecuteProcess> GetProcess(const std::string& name) const;

  /**
   * @brief Check if a process is ready to start
   * @param name Process name
   * @param completed Set of already completed process names
   * @return true if all dependencies are satisfied
   */
  bool IsReady(const std::string& name, const std::set<std::string>& completed) const;

  /**
   * @brief Clear all registered processes
   */
  void Clear();

  /**
   * @brief Get number of registered processes
   */
  size_t GetProcessCount() const { return processes_.size(); }

 private:
  std::unordered_map<std::string, std::shared_ptr<ExecuteProcess>> processes_;
  std::unordered_map<std::string, std::vector<std::string>> dependencies_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__DEPENDENCY_MANAGER_HPP_
