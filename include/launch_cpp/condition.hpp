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


#ifndef LAUNCH_CPP__CONDITION_HPP_
#define LAUNCH_CPP__CONDITION_HPP_

// AUTOSAR C++14 Compliant Condition Interface

#include <string>
#include <memory>
#include <functional>

namespace launch_cpp
{

// Forward declaration
class LaunchContext;

// AUTOSAR C++14: A12-8-4 - Base class with virtual destructor
class Condition
{
 public:
  Condition() = default;

  // AUTOSAR C++14: A12-8-4 - Virtual destructor
  virtual ~Condition() {}

  // AUTOSAR C++14: A10-3-3 - Declare special functions
  Condition(const Condition&) = default;
  Condition& operator=(const Condition&) = default;
  Condition(Condition&&) = default;
  Condition& operator=(Condition&&) = default;

  // AUTOSAR C++14: M0-1-9 - Pure virtual function
  virtual bool evaluate(const LaunchContext& context) const = 0;

  // Convenience operator
  bool operator()(const LaunchContext& context) const
  {
    return evaluate(context);
  }
};

using ConditionPtr = std::shared_ptr<Condition>;

// Function-based condition
class FunctionCondition final : public Condition
{
 public:
  using EvalFunc = std::function<bool(const LaunchContext&)>;

  explicit FunctionCondition(EvalFunc func)
    : func_(std::move(func)) {}

  bool evaluate(const LaunchContext& context) const override
  {
    return func_(context);
  }

 private:
  EvalFunc func_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__CONDITION_HPP_
