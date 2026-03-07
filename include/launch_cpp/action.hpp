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
 * @file action.hpp
 * @brief Base Action class for launch_cpp
 *
 * @details Defines the Action base class which is the fundamental executable
 *          entity in the launch system. Actions can have conditions and
 *          execute arbitrary operations within the launch context.
 *
 * @ASIL ASIL B
 *
 * @purpose Provide the base abstraction for all launch actions
 *
 * @requirements
 * - REQ-LAUNCH-ACTION-001: Define executable action interface
 * - REQ-LAUNCH-ACTION-002: Support conditional execution
 * - REQ-LAUNCH-ACTION-003: Enable error propagation
 */

#ifndef LAUNCH_CPP__ACTION_HPP_
#define LAUNCH_CPP__ACTION_HPP_

#include "launch_cpp/launch_description_entity.hpp"
#include "launch_cpp/error_code.hpp"
#include "launch_cpp/condition.hpp"
#include <functional>
#include <memory>

namespace launch_cpp
{

// Forward declaration
class LaunchContext;

/**
 * @brief Base class for all launch actions
 *
 * @details The Action class provides the foundation for executable entities
 *          in the launch system. Derived classes implement specific behaviors
 *          such as process execution, timer management, or configuration.
 *
 * @note AUTOSAR C++14: A12-8-4 - Base class with virtual destructor
 * @note AUTOSAR C++14: A10-3-3 - Declare special member functions
 *
 * @requirements REQ-LAUNCH-ACTION-001
 */
class Action : public LaunchDescriptionEntity
{
 public:
  /**
   * @brief Default constructor
   *
   * @pre None
   * @post Action created with no condition
   *
   * @note AUTOSAR C++14: A12-1-1 - Use member initialization list
   */
  Action() : condition_(nullptr) {}
  
  /**
   * @brief Constructor with condition
   *
   * @param condition Condition for execution (may be nullptr)
   *
   * @pre None
   * @post Action created with specified condition
   */
  explicit Action(const ConditionPtr& condition)
    : condition_(condition) {}
  
  /**
   * @brief Virtual destructor
   *
   * @note AUTOSAR C++14: A12-8-4 - Virtual destructor required for base class
   * @note Ensures proper cleanup of derived classes
   */
  virtual ~Action() {}
  
  /**
   * @brief Copy constructor
   * @note AUTOSAR C++14: A10-3-3 - Explicitly declared special functions
   */
  Action(const Action&) = default;
  
  /**
   * @brief Copy assignment operator
   * @note AUTOSAR C++14: A10-3-3 - Explicitly declared special functions
   */
  Action& operator=(const Action&) = default;
  
  /**
   * @brief Move constructor
   * @note AUTOSAR C++14: A10-3-3 - Explicitly declared special functions
   */
  Action(Action&&) = default;
  
  /**
   * @brief Move assignment operator
   * @note AUTOSAR C++14: A10-3-3 - Explicitly declared special functions
   */
  Action& operator=(Action&&) = default;
  
  /**
   * @brief Visit the action in the launch context
   *
   * @param context The launch context for execution
   * @return Result containing child entities or error
   *
   * @pre context must be valid and initialized
   * @post If condition evaluates false, returns empty vector
   * @post If Execute() succeeds, returns empty vector
   * @post If Execute() fails, returns error result
   *
   * @note AUTOSAR C++14: M0-1-9 - Override Visit from base
   * @note This method implements the visitor pattern
   *
   * @thread_safety Not thread-safe - call only from LaunchService thread
   *
   * @requirements REQ-LAUNCH-ACTION-001, REQ-LAUNCH-ACTION-002
   */
  Result<LaunchDescriptionEntityVector> Visit(LaunchContext& context) override;
  
  /**
   * @brief Execute the action
   *
   * @param context The launch context for execution
   * @return Result indicating success or failure
   *
   * @pre context must be valid and initialized
   * @post Action-specific behavior executed
   * @post On success, returns void result
   * @post On failure, returns error with diagnostic message
   *
   * @note AUTOSAR C++14: M0-1-9 - Pure virtual execute
   * @note Derived classes must implement this method
   * @note May perform I/O operations (process spawn, file access)
   *
   * @thread_safety Not thread-safe - call only from LaunchService thread
   *
   * @requirements REQ-LAUNCH-ACTION-001
   */
  virtual Result<void> Execute(LaunchContext& context) = 0;
  
  /**
   * @brief Check if action has a condition
   *
   * @return true if condition is set
   *
   * @pre None
   * @post Returns state of condition pointer
   *
   * @note AUTOSAR C++14: M0-1-9 - Declare functions as noexcept
   *
   * @thread_safety Thread-safe for read-only access
   */
  bool HasCondition() const noexcept { return condition_ != nullptr; }
  
  /**
   * @brief Get the condition
   *
   * @return Reference to condition pointer (may be nullptr)
   *
   * @pre None
   * @post Returns reference to internal condition pointer
   *
   * @note Check HasCondition() before use
   *
   * @thread_safety Thread-safe for read-only access
   */
  const ConditionPtr& GetCondition() const noexcept { return condition_; }
  
 protected:
  /**
   * @brief Set or update the condition
   *
   * @param condition New condition (may be nullptr to clear)
   *
   * @pre None
   * @post condition_ updated to new value
   *
   * @note Protected to allow derived classes to modify condition
   */
  void SetCondition(const ConditionPtr& condition) { condition_ = condition; }
  
 private:
  ConditionPtr condition_;  ///< Optional condition for conditional execution
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__ACTION_HPP_
