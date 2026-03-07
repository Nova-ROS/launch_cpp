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


#ifndef LAUNCH_CPP__TYPES_HPP_
#define LAUNCH_CPP__TYPES_HPP_

// AUTOSAR C++14 Compliant Header
// This file defines basic types and utilities for the launch_cpp library

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <utility>

namespace launch_cpp
{

// AUTOSAR C++14: A18-5-3 - Use std::make_unique for unique_ptr creation
template<typename T, typename... Args>
inline std::unique_ptr<T> MakeUnique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// AUTOSAR C++14: A18-5-1 - Use std::make_shared for shared_ptr creation
template<typename T, typename... Args>
inline std::shared_ptr<T> MakeShared(Args&&... args)
{
  return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
}

// Forward declarations
class LaunchService;
class LaunchContext;
class LaunchDescription;
class LaunchDescriptionEntity;
class Action;
class Event;
class EventHandler;
class Substitution;
class Condition;

// Smart pointer aliases
using LaunchDescriptionPtr = std::shared_ptr<LaunchDescription>;
using LaunchDescriptionEntityPtr = std::shared_ptr<LaunchDescriptionEntity>;
using ActionPtr = std::shared_ptr<Action>;
using EventPtr = std::shared_ptr<Event>;
using EventHandlerPtr = std::shared_ptr<EventHandler>;
using SubstitutionPtr = std::shared_ptr<Substitution>;
using ConditionPtr = std::shared_ptr<Condition>;

// Vector aliases
using LaunchDescriptionEntityVector = std::vector<LaunchDescriptionEntityPtr>;
using ActionVector = std::vector<ActionPtr>;
using EventHandlerVector = std::vector<EventHandlerPtr>;
using SubstitutionVector = std::vector<SubstitutionPtr>;

// AUTOSAR C++14: M5-0-3 - Use explicit type conversions
// Type-safe cast wrappers
template<typename To, typename From>
inline To* PolymorphicCast(From* ptr)
{
  return dynamic_cast<To*>(ptr);
}

template<typename To, typename From>
inline const To* PolymorphicCast(const From* ptr)
{
  return dynamic_cast<const To*>(ptr);
}

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__TYPES_HPP_
