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
 * @file launch_context.hpp
 * @brief Launch context interface for execution environment
 *
 * @details The LaunchContext provides the execution environment for actions
 *          and maintains state across the launch system including event
 *          handlers, configurations, and environment variables.
 *
 * @ASIL ASIL B
 *
 * @purpose Define the context interface for launch execution
 *
 * @requirements
 * - REQ-LAUNCH-CONTEXT-001: Provide configuration storage and retrieval
 * - REQ-LAUNCH-CONTEXT-002: Support event handler registration
 * - REQ-LAUNCH-CONTEXT-003: Manage environment variables
 * - REQ-LAUNCH-CONTEXT-004: Track current launch file
 */

#ifndef LAUNCH_CPP__LAUNCH_CONTEXT_HPP_
#define LAUNCH_CPP__LAUNCH_CONTEXT_HPP_

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "launch_cpp/types.hpp"
#include "launch_cpp/error_code.hpp"
#include "launch_cpp/event.hpp"
#include "launch_cpp/event_handler.hpp"

namespace launch_cpp
{

/**
 * @brief Launch execution context interface
 *
 * @details Provides the runtime environment for launch actions.
 *          Manages configurations, event handlers, and environment state.
 *          This is an abstract interface - concrete implementation in
 *          launch_context_impl.hpp
 *
 * @note AUTOSAR C++14: A12-8-4 - Base class with virtual destructor
 * @note AUTOSAR C++14: A10-3-3 - Declare special functions
 *
 * @requirements REQ-LAUNCH-CONTEXT-001, REQ-LAUNCH-CONTEXT-002
 */
class LaunchContext
{
 public:
  /**
   * @brief Launch arguments structure
   *
   * @details Contains command-line arguments and execution options
   *          passed to the launch system.
   */
  struct Arguments
  {
    std::vector<std::string> argv;  ///< Command-line arguments
    bool noninteractive = false;    ///< Non-interactive mode flag
  };
  
  /**
   * @brief Default constructor
   * @note Creates empty context
   */
  LaunchContext() = default;
  
  /**
   * @brief Virtual destructor
   *
   * @note AUTOSAR C++14: A12-8-4 - Virtual destructor required
   * @note Ensures proper cleanup of derived implementations
   */
  virtual ~LaunchContext() {}
  
  /**
   * @brief Copy constructor (deleted)
   * @note Contexts are non-copyable to prevent state duplication
   */
  LaunchContext(const LaunchContext&) = delete;
  
  /**
   * @brief Copy assignment operator (deleted)
   * @note Contexts are non-copyable
   */
  LaunchContext& operator=(const LaunchContext&) = delete;
  
  /**
   * @brief Move constructor (deleted)
   * @note Contexts should not be moved during execution
   */
  LaunchContext(LaunchContext&&) = delete;
  
  /**
   * @brief Move assignment operator (deleted)
   * @note Contexts should not be moved during execution
   */
  LaunchContext& operator=(LaunchContext&&) = delete;
  
  /**
   * @brief Register an event handler
   *
   * @param handler Shared pointer to event handler
   *
   * @pre handler must not be nullptr
   * @post handler registered for event dispatch
   *
   * @note Handler will receive all events emitted via EmitEvent()
   * @note Duplicate registrations are allowed but may cause duplicate handling
   *
   * @thread_safety Thread-safe
   *
   * @requirements REQ-LAUNCH-CONTEXT-002
   */
  virtual void RegisterEventHandler(const EventHandlerPtr& handler) = 0;
  
  /**
   * @brief Unregister an event handler
   *
   * @param handler Pointer to handler to unregister (raw pointer for comparison)
   *
   * @pre handler must have been previously registered
   * @post handler removed from registry
   *
   * @note Uses raw pointer for identity comparison
   * @note Safe to call with unregistered handler (no-op)
   *
   * @thread_safety Thread-safe
   *
   * @requirements REQ-LAUNCH-CONTEXT-002
   */
  virtual void UnregisterEventHandler(const EventHandler* handler) = 0;
  
  /**
   * @brief Get all registered event handlers
   *
   * @return Reference to vector of event handlers
   *
   * @pre None
   * @post Returns current list of handlers
   *
   * @note Reference remains valid only during call
   * @note Do not modify the returned vector
   *
   * @thread_safety Thread-safe for read-only access
   *
   * @requirements REQ-LAUNCH-CONTEXT-002
   */
  virtual const EventHandlerVector& GetEventHandlers() const = 0;
  
  /**
   * @brief Set a launch configuration value
   *
   * @param key Configuration key
   * @param value Configuration value
   *
   * @pre None
   * @post Configuration value stored (overwrites existing)
   *
   * @note Keys are case-sensitive
   * @note Empty values are allowed
   *
   * @thread_safety Thread-safe
   *
   * @requirements REQ-LAUNCH-CONTEXT-001
   */
  virtual void SetLaunchConfiguration(const std::string& key, const std::string& value) = 0;
  
  /**
   * @brief Get a launch configuration value
   *
   * @param key Configuration key to retrieve
   * @return Result containing value or error (kInvalidConfiguration if not found)
   *
   * @pre None
   * @post Returns value if key exists
   * @post Returns error if key not found
   *
   * @note Check HasLaunchConfiguration() before calling to avoid errors
   *
   * @thread_safety Thread-safe
   *
   * @requirements REQ-LAUNCH-CONTEXT-001
   */
  virtual Result<std::string> GetLaunchConfiguration(const std::string& key) const = 0;
  
  /**
   * @brief Check if a configuration key exists
   *
   * @param key Configuration key to check
   * @return true if key exists
   *
   * @pre None
   * @post Returns existence status
   *
   * @thread_safety Thread-safe
   *
   * @requirements REQ-LAUNCH-CONTEXT-001
   */
  virtual bool HasLaunchConfiguration(const std::string& key) const = 0;
  
  /**
   * @brief Set the current launch file path
   *
   * @param path Absolute path to current launch file
   *
   * @pre path should be absolute
   * @post Current launch file path updated
   *
   * @note Used by substitutions like ThisLaunchFile
   *
   * @thread_safety Thread-safe
   *
   * @requirements REQ-LAUNCH-CONTEXT-004
   */
  virtual void SetCurrentLaunchFile(const std::string& path) = 0;
  
  /**
   * @brief Get the current launch file path
   *
   * @return Path to current launch file (empty if none)
   *
   * @pre None
   * @post Returns stored path or empty string
   *
   * @thread_safety Thread-safe
   *
   * @requirements REQ-LAUNCH-CONTEXT-004
   */
  virtual std::string GetCurrentLaunchFile() const = 0;
  
  /**
   * @brief Get an environment variable
   *
   * @param name Environment variable name
   * @return Value of environment variable (empty if not set)
   *
   * @pre None
   * @post Returns variable value or empty string
   *
   * @note Uses std::getenv internally
   * @note Empty return does not distinguish between empty value and unset
   *
   * @thread_safety Thread-safe (with underlying OS guarantees)
   *
   * @requirements REQ-LAUNCH-CONTEXT-003
   */
  virtual std::string GetEnvironmentVariable(const std::string& name) const = 0;
  
  /**
   * @brief Set an environment variable
   *
   * @param name Environment variable name
   * @param value Environment variable value
   *
   * @pre name must not be empty
   * @post Environment variable set in current process
   *
   * @note Uses setenv/putenv internally
   * @note Affects child processes spawned after this call
   *
   * @thread_safety Not thread-safe with respect to getenv
   *
   * @requirements REQ-LAUNCH-CONTEXT-003
   */
  virtual void SetEnvironmentVariable(const std::string& name, const std::string& value) = 0;
  
  /**
   * @brief Emit an event to all registered handlers
   *
   * @param event Event to emit (ownership transferred)
   *
   * @pre event must not be nullptr
   * @post event dispatched to all handlers
   *
   * @note Handlers are called synchronously
   * @note Exceptions in handlers may propagate
   *
   * @thread_safety Thread-safe
   *
   * @requirements REQ-LAUNCH-CONTEXT-002
   */
  virtual void EmitEvent(EventPtr event) = 0;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__LAUNCH_CONTEXT_HPP_
