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

#include "launch_cpp/dependency_manager.hpp"
#include <algorithm>

namespace launch_cpp
{

Error DependencyManager::add_process(
    const std::string& name,
    const std::shared_ptr<ExecuteProcess>& action,
    const std::vector<std::string>& dependencies)
{
  if (name.empty())
  {
    return Error(ErrorCode::kInvalidArgument, "Process name cannot be empty");
  }

  if (!action)
  {
    return Error(ErrorCode::kInvalidArgument, "Process action cannot be null");
  }

  // Check for duplicate
  if (processes_.find(name) != processes_.end())
  {
    return Error(ErrorCode::kInvalidConfiguration,
                 "Process '" + name + "' already exists");
  }

  processes_[name] = action;
  dependencies_[name] = dependencies;

  return Error();  // Success
}

ResolutionResult DependencyManager::resolve_dependencies() const
{
  // Build NodeConfig list
  std::vector<NodeConfig> nodes;
  for (const auto& [name, deps] : dependencies_)
  {
    NodeConfig config;
    config.name = name;
    config.dependencies = deps;
    nodes.push_back(config);
  }

  DependencyResolver resolver;
  return resolver.resolve(nodes);
}

Error DependencyManager::execute_all(LaunchContext& context)
{
  // Resolve dependencies
  auto result = resolve_dependencies();
  if (!result.success)
  {
    return Error(ErrorCode::kCyclicDependency, result.error_message);
  }

  // Execute in order
  std::set<std::string> completed;

  for (const auto& process_name : result.order)
  {
    auto it = processes_.find(process_name);
    if (it == processes_.end())
    {
      return Error(ErrorCode::kProcessNotFound,
                   "Process '" + process_name + "' not found");
    }

    // Check if ready
    if (!is_ready(process_name, completed))
    {
      return Error(ErrorCode::kCyclicDependency,
                   "Dependencies not satisfied for '" + process_name + "'");
    }

    // Execute the process
    auto exec_result = it->second->execute(context);
    if (exec_result.has_error())
    {
      return exec_result.get_error();
    }

    completed.insert(process_name);
  }

  return Error();  // Success
}

std::shared_ptr<ExecuteProcess> DependencyManager::get_process(
    const std::string& name) const
{
  auto it = processes_.find(name);
  if (it != processes_.end())
  {
    return it->second;
  }
  return nullptr;
}

bool DependencyManager::is_ready(
    const std::string& name,
    const std::set<std::string>& completed) const
{
  auto it = dependencies_.find(name);
  if (it == dependencies_.end())
  {
    return false;  // Process not found
  }

  // Check all dependencies are completed
  for (const auto& dep : it->second)
  {
    if (completed.find(dep) == completed.end())
    {
      return false;
    }
  }

  return true;
}

void DependencyManager::clear()
{
  processes_.clear();
  dependencies_.clear();
}

}  // namespace launch_cpp
