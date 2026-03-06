// Copyright 2024 Example Author
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

#ifndef CPP_LAUNCH__LAUNCH_DESCRIPTION_HPP_
#define CPP_LAUNCH__LAUNCH_DESCRIPTION_HPP_

// AUTOSAR C++14 Compliant Launch Description

#include "cpp_launch/launch_description_entity.hpp"
#include "cpp_launch/types.hpp"
#include "cpp_launch/error_code.hpp"
#include <vector>
#include <string>

namespace cpp_launch
{

// Forward declaration for YAML parsing
class YamlNode;

// AUTOSAR C++14: M3-2-1 - Use explicit for single-parameter constructors
class LaunchDescription final : public LaunchDescriptionEntity
{
 public:
  // AUTOSAR C++14: A12-1-1 - Default constructor
  LaunchDescription() = default;
  
  explicit LaunchDescription(const LaunchDescriptionEntityVector& entities);
  explicit LaunchDescription(LaunchDescriptionEntityVector&& entities);
  
  // AUTOSAR C++14: A12-8-4 - Virtual destructor
  ~LaunchDescription() override = default;
  
  // AUTOSAR C++14: A10-3-3 - Declare special functions
  LaunchDescription(const LaunchDescription&) = default;
  LaunchDescription& operator=(const LaunchDescription&) = default;
  LaunchDescription(LaunchDescription&&) = default;
  LaunchDescription& operator=(LaunchDescription&&) = default;
  
  // Add entities
  void Add(const LaunchDescriptionEntityPtr& entity);
  void Add(LaunchDescriptionEntityPtr&& entity);
  
  template<typename T, typename... Args>
  void Emplace(Args&&... args)
  {
    Add(MakeShared<T>(std::forward<Args>(args)...));
  }
  
  // AUTOSAR C++14: M0-1-9 - Override Visit
  Result<LaunchDescriptionEntityVector> Visit(LaunchContext& context) override;
  
  // Getters
  const LaunchDescriptionEntityVector& GetEntities() const noexcept { return entities_; }
  
  // Factory methods
  static Result<LaunchDescriptionPtr> FromYaml(const std::string& yamlString);
  static Result<LaunchDescriptionPtr> FromYamlFile(const std::string& filePath);
  
 private:
  LaunchDescriptionEntityVector entities_;
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__LAUNCH_DESCRIPTION_HPP_
