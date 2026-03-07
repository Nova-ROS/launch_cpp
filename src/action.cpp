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
 * @file action.cpp
 * @brief Action class implementation
 *
 * @details Implements the Action base class Visit() method which handles
 *          condition evaluation and delegates to Execute().
 *
 * @ASIL ASIL B
 *
 * @purpose Implement action execution with conditional support
 *
 * @requirements
 * - REQ-LAUNCH-ACTION-001: Execute actions with proper error handling
 * - REQ-LAUNCH-ACTION-002: Evaluate conditions before execution
 */

#include "launch_cpp/action.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/condition.hpp"

namespace launch_cpp
{

/**
 * @brief Visit the action and execute if conditions are met
 *
 * @details This method implements the visitor pattern for actions.
 *          It evaluates any attached condition, and if true (or no condition),
 *          executes the action. Error handling propagates errors from both
 *          condition evaluation and action execution.
 *
 * @param context The launch context for execution
 * @return Result containing empty vector on success, or error on failure
 *
 * @pre context must be valid and initialized
 * @post If condition evaluates false, returns empty entity vector
 * @post If Execute() succeeds, returns empty entity vector
 * @post If Execute() fails, returns error with diagnostic information
 *
 * @note This is the base class implementation - derived classes override Execute()
 * @note Returns empty vector because actions don't spawn child entities directly
 *
 * @thread_safety Not thread-safe - must be called from LaunchService thread only
 *
 * @safety_critical No - condition evaluation and execution must not block
 * @safety_note All error conditions must be handled and returned, not thrown
 *
 * @requirements REQ-LAUNCH-ACTION-001, REQ-LAUNCH-ACTION-002
 */
Result<LaunchDescriptionEntityVector> Action::Visit(LaunchContext& context)
{
  // Check if this action has a condition attached
  // Safety: Condition evaluation should not throw
  if (HasCondition())
  {
    // Evaluate the condition in the given context
    // Requirements: REQ-LAUNCH-ACTION-002
    bool conditionResult = GetCondition()->Evaluate(context);
    
    // If condition evaluates to false, skip execution
    // Return empty vector indicating no action taken
    if (!conditionResult)
    {
      return Result<LaunchDescriptionEntityVector>(LaunchDescriptionEntityVector{});
    }
  }
  
  // Condition passed (or no condition), execute the action
  // Delegate to pure virtual Execute() implemented by derived class
  // Requirements: REQ-LAUNCH-ACTION-001
  Result<void> executeResult = Execute(context);
  
  // Check for execution errors
  // Safety: Must propagate errors without exceptions
  if (executeResult.HasError())
  {
    // Propagate error from Execute()
    // Create result with same error code and message
    Result<LaunchDescriptionEntityVector> errorResult(executeResult.GetError());
    return errorResult;
  }
  
  // Execution succeeded, return empty vector
  // Actions don't directly return child entities
  return Result<LaunchDescriptionEntityVector>(LaunchDescriptionEntityVector{});
}

}  // namespace launch_cpp
