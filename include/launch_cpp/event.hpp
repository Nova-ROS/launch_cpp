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
 * @file event.hpp
 * @brief Event base class and built-in event types
 *
 * @details Defines the Event base class for the publish-subscribe event
 *          system, along with built-in event types for process lifecycle
 *          and system events.
 *
 * @ASIL ASIL B
 *
 * @purpose Define event types for inter-component communication
 *
 * @requirements
 * - REQ-LAUNCH-EVENT-001: Define event base class
 * - REQ-LAUNCH-EVENT-002: Provide built-in event types
 * - REQ-LAUNCH-EVENT-003: Support event timestamping
 */

#ifndef LAUNCH_CPP__EVENT_HPP_
#define LAUNCH_CPP__EVENT_HPP_

#include <chrono>
#include <string>
#include <cstdint>
#include "launch_cpp/types.hpp"

namespace launch_cpp
{

/**
 * @brief Base class for all events
 *
 * @details Provides the foundation for the event system.
 *          All event types must derive from this class.
 *          Events are timestamped at creation.
 *
 * @note AUTOSAR C++14: A12-8-4 - Base class with virtual destructor
 * @note AUTOSAR C++14: A10-3-3 - Declare special functions
 * @note Events are immutable after creation
 *
 * @requirements REQ-LAUNCH-EVENT-001
 */
class Event
{
 public:
  /**
   * @brief Default constructor
   *
   * @post Event created with current timestamp
   * @note AUTOSAR C++14: A12-1-1 - Use member initialization list
   */
  Event() : timestamp_(std::chrono::steady_clock::now()) {}
  
  /**
   * @brief Virtual destructor
   *
   * @note AUTOSAR C++14: A12-8-4 - Virtual destructor required
   * @note Enables polymorphic deletion
   */
  virtual ~Event() {}
  
  /**
   * @brief Copy constructor
   * @note AUTOSAR C++14: A10-3-3 - Explicitly declared
   */
  Event(const Event&) = default;
  
  /**
   * @brief Copy assignment operator
   * @note AUTOSAR C++14: A10-3-3 - Explicitly declared
   */
  Event& operator=(const Event&) = default;
  
  /**
   * @brief Move constructor
   * @note AUTOSAR C++14: A10-3-3 - Explicitly declared
   */
  Event(Event&&) = default;
  
  /**
   * @brief Move assignment operator
   * @note AUTOSAR C++14: A10-3-3 - Explicitly declared
   */
  Event& operator=(Event&&) = default;
  
  /**
   * @brief Get the event type identifier
   *
   * @return C-string type identifier
   *
   * @pre None
   * @post Returns non-null type string
   *
   * @note Derived classes must implement this
   * @note Return value must be static string (lifetime > event)
   *
   * @note AUTOSAR C++14: M0-1-9 - Pure virtual function
   *
   * @requirements REQ-LAUNCH-EVENT-001
   */
  virtual const char* get_type() const = 0;

  /**
   * @brief Get the event creation timestamp
   *
   * @return Time point when event was created
   *
   * @pre None
   * @post Returns stored timestamp
   *
   * @note AUTOSAR C++14: M0-1-9 - Declare noexcept
   * @note Uses steady_clock for monotonic timing
   *
   * @requirements REQ-LAUNCH-EVENT-003
   */
  std::chrono::steady_clock::time_point get_timestamp() const noexcept
  {
    return timestamp_;
  }
  
 protected:
  std::chrono::steady_clock::time_point timestamp_;  ///< Creation timestamp
};

/**
 * @brief Event emitted when a process starts
 *
 * @details Contains process ID and process name.
 *          Emitted by ExecuteProcess action.
 *
 * @requirements REQ-LAUNCH-EVENT-002
 */
class ProcessStartedEvent final : public Event
{
 public:
  /**
   * @brief Constructor
   *
   * @param pid Process ID of started process
   * @param name Process name/identifier
   * @post Event initialized with process info
   */
  ProcessStartedEvent(std::int32_t pid, const std::string& name)
    : pid_(pid), name_(name) {}
  
  /**
   * @brief Get event type
   * @return "process_started"
   */
  const char* get_type() const override { return "process_started"; }

  /**
   * @brief Get the process ID
   * @return Process ID
   */
  std::int32_t get_pid() const noexcept { return pid_; }

  /**
   * @brief Get the process name
   * @return Process name
   */
  const std::string& get_name() const noexcept { return name_; }
  
 private:
  std::int32_t pid_;      ///< Process identifier
  std::string name_;      ///< Process name
};

/**
 * @brief Event emitted when a process exits
 *
 * @details Contains process ID, exit code, and process name.
 *          Emitted when monitored process terminates.
 *
 * @requirements REQ-LAUNCH-EVENT-002
 */
class ProcessExitedEvent final : public Event
{
 public:
  /**
   * @brief Constructor
   *
   * @param pid Process ID of exited process
   * @param returnCode Process exit code
   * @param name Process name/identifier
   * @post Event initialized with process exit info
   */
  ProcessExitedEvent(std::int32_t pid, std::int32_t returnCode, const std::string& name)
    : pid_(pid), return_code_(returnCode), name_(name) {}
  
  /**
   * @brief Get event type
   * @return "process_exited"
   */
  const char* get_type() const override { return "process_exited"; }

  /**
   * @brief Get the process ID
   * @return Process ID
   */
  std::int32_t get_pid() const noexcept { return pid_; }

  /**
   * @brief Get the exit code
   * @return Exit code (0 = success, non-zero = error)
   */
  std::int32_t get_return_code() const noexcept { return return_code_; }

  /**
   * @brief Get the process name
   * @return Process name
   */
  const std::string& get_name() const noexcept { return name_; }
  
 private:
  std::int32_t pid_;          ///< Process identifier
  std::int32_t return_code_;   ///< Exit status code
  std::string name_;          ///< Process name
};

/**
 * @brief Event emitted when shutdown is requested
 *
 * @details Signals that the launch system should shut down.
 *          Contains reason for shutdown.
 *
 * @requirements REQ-LAUNCH-EVENT-002
 */
class ShutdownEvent final : public Event
{
 public:
  /**
   * @brief Constructor
   *
   * @param reason Reason for shutdown
   * @post Event initialized with shutdown reason
   */
  explicit ShutdownEvent(const std::string& reason)
    : reason_(reason) {}
  
  /**
   * @brief Get event type
   * @return "shutdown"
   */
  const char* get_type() const override { return "shutdown"; }

  /**
   * @brief Get the shutdown reason
   * @return Shutdown reason description
   */
  const std::string& get_reason() const noexcept { return reason_; }
  
 private:
  std::string reason_;  ///< Shutdown reason
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__EVENT_HPP_
