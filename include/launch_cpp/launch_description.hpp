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


#ifndef LAUNCH_CPP__LAUNCH_DESCRIPTION_HPP_
#define LAUNCH_CPP__LAUNCH_DESCRIPTION_HPP_

// AUTOSAR C++14 Compliant Launch Description

#include "launch_cpp/launch_description_entity.hpp"
#include "launch_cpp/types.hpp"
#include "launch_cpp/error_code.hpp"
#include <vector>
#include <string>

namespace launch_cpp
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
  void add(const LaunchDescriptionEntityPtr& entity);
  void add(LaunchDescriptionEntityPtr&& entity);

  template<typename T, typename... Args>
  void emplace(Args&&... args)
  {
    add(MakeShared<T>(std::forward<Args>(args)...));
  }

  // AUTOSAR C++14: M0-1-9 - Override Visit
  Result<LaunchDescriptionEntityVector> visit(LaunchContext& context) override;

  // Getters
  const LaunchDescriptionEntityVector& get_entities() const noexcept { return entities_; }

  // Factory methods
  static Result<LaunchDescriptionPtr> from_yaml(const std::string& yaml_string);
  static Result<LaunchDescriptionPtr> from_yaml_file(const std::string& file_path);

 private:
  LaunchDescriptionEntityVector entities_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__LAUNCH_DESCRIPTION_HPP_
