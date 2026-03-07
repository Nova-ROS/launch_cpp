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


#ifndef CPP_LAUNCH__ACTION_HPP_
#define CPP_LAUNCH__ACTION_HPP_

// AUTOSAR C++14 Compliant Action Base Class

#include "cpp_launch/launch_description_entity.hpp"
#include "cpp_launch/error_code.hpp"
#include "cpp_launch/condition.hpp"
#include <functional>
#include <memory>

namespace cpp_launch
{

// Forward declaration
class LaunchContext;

// AUTOSAR C++14: A12-8-4 - Base class with virtual destructor
class Action : public LaunchDescriptionEntity
{
 public:
  // AUTOSAR C++14: A12-1-1 - Use member initialization list
  Action() : condition_(nullptr) {}
  
  explicit Action(const ConditionPtr& condition)
    : condition_(condition) {}
  
  // AUTOSAR C++14: A12-8-4 - Virtual destructor
  virtual ~Action() {}
  
  // AUTOSAR C++14: A10-3-3 - Declare special functions
  Action(const Action&) = default;
  Action& operator=(const Action&) = default;
  Action(Action&&) = default;
  Action& operator=(Action&&) = default;
  
  // AUTOSAR C++14: M0-1-9 - Override Visit from base
  Result<LaunchDescriptionEntityVector> Visit(LaunchContext& context) override;
  
  // AUTOSAR C++14: M0-1-9 - Pure virtual execute
  virtual Result<void> Execute(LaunchContext& context) = 0;
  
  // AUTOSAR C++14: M0-1-9 - Getter
  bool HasCondition() const noexcept { return condition_ != nullptr; }
  
  const ConditionPtr& GetCondition() const noexcept { return condition_; }
  
 protected:
  void SetCondition(const ConditionPtr& condition) { condition_ = condition; }
  
 private:
  ConditionPtr condition_;
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__ACTION_HPP_
