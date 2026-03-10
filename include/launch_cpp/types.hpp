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
 * @file types.hpp
 * @brief Type definitions and smart pointer utilities for launch_cpp
 *
 * @details This header provides fundamental type definitions, smart pointer
 *          aliases, and casting utilities used throughout the launch_cpp
 *          library. All types are compliant with AUTOSAR C++14 guidelines.
 *
 * @ASIL ASIL B
 *
 * @purpose Define core types and memory management utilities for the
 *          launch_cpp framework
 *
 * @requirements
 * - REQ-LAUNCH-TYPES-001: Provide type-safe smart pointer creation
 * - REQ-LAUNCH-TYPES-002: Support polymorphic type casting
 * - REQ-LAUNCH-TYPES-003: Enable forward declarations for circular dependencies
 */

#ifndef LAUNCH_CPP__TYPES_HPP_
#define LAUNCH_CPP__TYPES_HPP_

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <utility>

namespace launch_cpp
{

/**
 * @brief Creates a unique_ptr with type-safe allocation
 *
 * @tparam T Type to allocate
 * @tparam Args Constructor argument types
 * @param args Constructor arguments
 * @return std::unique_ptr<T> Managed pointer to allocated object
 *
 * @pre None
 * @post Returns valid unique_ptr containing new T instance
 *
 * @note AUTOSAR C++14: A18-5-3 - Use std::make_unique for unique_ptr creation
 * @note May throw std::bad_alloc on allocation failure
 *
 * @requirements REQ-LAUNCH-TYPES-001
 */
template<typename T, typename... Args>
inline std::unique_ptr<T> MakeUnique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

/**
 * @brief Creates a shared_ptr with type-safe allocation
 *
 * @tparam T Type to allocate
 * @tparam Args Constructor argument types
 * @param args Constructor arguments
 * @return std::shared_ptr<T> Managed pointer to allocated object
 *
 * @pre None
 * @post Returns valid shared_ptr containing new T instance
 *
 * @note AUTOSAR C++14: A18-5-1 - Use std::make_shared for shared_ptr creation
 * @note May throw std::bad_alloc on allocation failure
 *
 * @requirements REQ-LAUNCH-TYPES-001
 */
template<typename T, typename... Args>
inline std::shared_ptr<T> MakeShared(Args&&... args)
{
  return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
}

// Forward declarations
class LaunchService;          ///< Main launch system service
class LaunchContext;          ///< Context for launch operations
class LaunchDescription;      ///< Launch description container
class LaunchDescriptionEntity;///< Base entity in launch description
class Action;                 ///< Executable action
class Event;                  ///< Event notification
class EventHandler;           ///< Event handler interface
class Substitution;           ///< Text substitution
class Condition;              ///< Condition evaluator

/**
 * @brief Smart pointer aliases for launch_cpp types
 *
 * @details These aliases provide consistent smart pointer usage
 *          across the codebase. Using shared_ptr enables shared
 *          ownership of launch entities.
 *
 * @note All pointers are nullable - always check before dereferencing
 * @note AUTOSAR C++14: A18-5-1, A18-5-3 - Smart pointer usage
 */
using LaunchDescriptionPtr = std::shared_ptr<LaunchDescription>;
using LaunchDescriptionEntityPtr = std::shared_ptr<LaunchDescriptionEntity>;
using ActionPtr = std::shared_ptr<Action>;
using EventPtr = std::shared_ptr<Event>;
using EventHandlerPtr = std::shared_ptr<EventHandler>;
using SubstitutionPtr = std::shared_ptr<Substitution>;
using ConditionPtr = std::shared_ptr<Condition>;

/**
 * @brief Vector type aliases for collections
 *
 * @details Provide type-safe containers for launch entities.
 *          Using vector for sequential access patterns.
 */
using LaunchDescriptionEntityVector = std::vector<LaunchDescriptionEntityPtr>;
using ActionVector = std::vector<ActionPtr>;
using EventHandlerVector = std::vector<EventHandlerPtr>;
using SubstitutionVector = std::vector<SubstitutionPtr>;

/**
 * @brief Performs safe polymorphic downcast
 *
 * @tparam To Target type (must be polymorphic)
 * @tparam From Source type (must be polymorphic)
 * @param ptr Pointer to cast
 * @return To* Casted pointer or nullptr if cast fails
 *
 * @pre ptr must point to a valid object or be nullptr
 * @post Returns nullptr if cast is invalid
 *
 * @note AUTOSAR C++14: M5-0-3 - Use explicit type conversions
 * @note Always check return value before use
 *
 * @thread_safety Thread-safe for read-only access
 *
 * @requirements REQ-LAUNCH-TYPES-002
 */
template<typename To, typename From>
inline To* polymorphic_cast(From* ptr)
{
  return dynamic_cast<To*>(ptr);
}

/**
 * @brief Performs safe polymorphic downcast (const version)
 *
 * @tparam To Target type (must be polymorphic)
 * @tparam From Source type (must be polymorphic)
 * @param ptr Pointer to cast
 * @return const To* Casted pointer or nullptr if cast fails
 *
 * @pre ptr must point to a valid object or be nullptr
 * @post Returns nullptr if cast is invalid
 *
 * @note AUTOSAR C++14: M5-0-3 - Use explicit type conversions
 * @note Always check return value before use
 *
 * @thread_safety Thread-safe for read-only access
 *
 * @requirements REQ-LAUNCH-TYPES-002
 */
template<typename To, typename From>
inline const To* polymorphic_cast(const From* ptr)
{
  return dynamic_cast<const To*>(ptr);
}

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__TYPES_HPP_
