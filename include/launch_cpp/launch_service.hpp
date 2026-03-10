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
 * @file launch_service.hpp
 * @brief LaunchService interface - Main orchestration component
 *
 * @details The LaunchService is the central component that manages the lifecycle
 *          of launch descriptions and coordinates action execution. It maintains
 *          the launch context, handles state transitions, and provides thread-safe
 *          access to launch system resources.
 *
 * @ASIL ASIL B
 *
 * @purpose Provide the main entry point for launch system execution
 *
 * @requirements
 * - REQ-LAUNCH-SERVICE-001: Manage launch description lifecycle
 * - REQ-LAUNCH-SERVICE-002: Execute actions in context
 * - REQ-LAUNCH-SERVICE-003: Handle graceful shutdown
 * - REQ-LAUNCH-SERVICE-004: Provide thread-safe state management
 */

#ifndef LAUNCH_CPP__LAUNCH_SERVICE_HPP_
#define LAUNCH_CPP__LAUNCH_SERVICE_HPP_

#include "launch_cpp/types.hpp"
#include "launch_cpp/error_code.hpp"
#include "launch_cpp/launch_description.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace launch_cpp
{

// Forward declarations
class LaunchContext;
class Event;
class EventHandler;

/**
 * @brief Launch service states
 *
 * @details Enumerates the possible states of the launch service.
 *          State transitions: kIdle -> kRunning -> kShuttingDown -> kStopped
 *
 * @note AUTOSAR C++14: A7-2-4 - Use enum class for strong typing
 *
 * @requirements REQ-LAUNCH-SERVICE-004
 */
enum class LaunchServiceStatus : std::int32_t
{
  kIdle = 0,          ///< Initial state, ready to run
  kRunning = 1,       ///< Active, processing launch descriptions
  kShuttingDown = 2,  ///< Shutdown in progress
  kStopped = 3        ///< Final state, resources released
};

/**
 * @brief Main launch service class
 *
 * @details Coordinates the execution of launch descriptions and manages
 *          the lifecycle of launched processes. Provides thread-safe
 *          state management and graceful shutdown capabilities.
 *
 *          Key responsibilities:
 *          - Managing launch descriptions
 *          - Executing actions in proper context
 *          - State machine management
 *          - Event handling coordination
 *
 * @note Non-copyable and non-movable
 * @note Thread-safe for concurrent status queries
 *
 * @ASIL ASIL B
 *
 * @requirements REQ-LAUNCH-SERVICE-001, REQ-LAUNCH-SERVICE-002,
 *               REQ-LAUNCH-SERVICE-003, REQ-LAUNCH-SERVICE-004
 */
class LaunchService final
{
 public:
  /**
   * @brief Service configuration options
   *
   * @details Parameters for configuring the launch service behavior.
   *          Passed to constructor.
   */
  struct Options
  {
    std::vector<std::string> argv;  ///< Command-line arguments
    bool noninteractive;            ///< Non-interactive mode flag
    bool debug;                     ///< Debug mode flag
    
    /**
     * @brief Default constructor
     * @post Options initialized with defaults (noninteractive=false, debug=false)
     */
    Options() : noninteractive(false), debug(false) {}
  };
  
  /**
   * @brief Construct launch service
   *
   * @param options Service configuration (uses defaults if not provided)
   * @post Service created in kIdle state
   *
   * @note Implementation uses Pimpl idiom for ABI compatibility
   *
   * @thread_safety Not thread-safe during construction
   *
   * @requirements REQ-LAUNCH-SERVICE-001
   */
  explicit LaunchService(const Options& options = Options());
  
  /**
   * @brief Destroy launch service
   *
   * @post Service shut down and resources released
   *
   * @note Automatically calls Shutdown() for clean termination
   * @note noexcept - must not throw
   *
   * @requirements REQ-LAUNCH-SERVICE-003
   */
  ~LaunchService() noexcept;
  
  /**
   * @brief Copy constructor (deleted)
   * @note Services are non-copyable to prevent state duplication
   */
  LaunchService(const LaunchService&) = delete;
  
  /**
   * @brief Copy assignment operator (deleted)
   * @note Services are non-copyable
   */
  LaunchService& operator=(const LaunchService&) = delete;
  
  /**
   * @brief Move constructor (deleted)
   * @note Services are non-movable
   */
  LaunchService(LaunchService&&) = delete;
  
  /**
   * @brief Move assignment operator (deleted)
   * @note Services are non-movable
   */
  LaunchService& operator=(LaunchService&&) = delete;
  
  /**
   * @brief Run the launch service
   *
   * @param shutdownWhenIdle If true, shut down after processing
   * @return Exit code (0 = success, non-zero = error)
   *
   * @pre Service must be in kIdle state
   * @post If successful, processes all registered launch descriptions
   * @post If shutdownWhenIdle, service shut down before return
   *
   * @note Main execution entry point
   * @note Not reentrant - single execution at a time
   *
   * @thread_safety Not thread-safe - single caller
   *
   * @safety_critical Yes - manages process lifecycle
   *
   * @requirements REQ-LAUNCH-SERVICE-001, REQ-LAUNCH-SERVICE-002
   */
  std::int32_t run(bool shutdown_when_idle = true);
  
  /**
   * @brief Add a launch description to be processed
   *
   * @param description Launch description to include
   * @return Error object indicating success or failure
   *
   * @pre description must not be nullptr
   * @post Description registered for execution
   *
   * @note Thread-safe - can be called before Run()
   * @note Descriptions executed in registration order
   *
   * @thread_safety Thread-safe
   *
   * @requirements REQ-LAUNCH-SERVICE-001
   */
  Error include_launch_description(const LaunchDescriptionPtr& description);

  /**
   * @brief Emit an event to registered handlers
   *
   * @param event Event to emit
   *
   * @pre None
   * @post Event dispatched (when implemented)
   *
   * @todo Implement event dispatch
   */
  void emit_event(EventPtr event);

  /**
   * @brief Shut down the launch service
   *
   * @return Error object indicating success or failure
   *
   * @pre May be called from any state
   * @post Service transitions to kStopped
   *
   * @note Initiates graceful shutdown
   * @note Idempotent - safe to call multiple times
   *
   * @thread_safety Thread-safe
   *
   * @safety_critical Yes - ensures clean termination
   *
   * @requirements REQ-LAUNCH-SERVICE-003
   */
  Error shutdown();

  /**
   * @brief Check if service is running
   *
   * @return true if in kRunning state
   *
   * @note Thread-safe via atomic operations
   *
   * @thread_safety Thread-safe
   */
  bool is_running() const noexcept;

  /**
   * @brief Check if service is idle
   *
   * @return true if in kIdle state
   *
   * @note Thread-safe via atomic operations
   *
   * @thread_safety Thread-safe
   */
  bool is_idle() const;
  
  /**
   * @brief Get current service status
   *
   * @return Current LaunchServiceStatus value
   *
   * @note Thread-safe via atomic load with acquire semantics
   *
   * @thread_safety Thread-safe
   */
  LaunchServiceStatus GetStatus() const noexcept
  {
    return status_.load(std::memory_order_acquire);
  }
  
  /**
   * @brief Get the launch context (non-const)
   *
   * @return Reference to launch context
   *
   * @pre Service must be valid
   * @post Returns reference to internal context
   *
   * @note Context lifetime tied to service
   *
   * @thread_safety Not thread-safe with context modification
   */
  LaunchContext& get_context();
  
  /**
   * @brief Get the launch context (const)
   *
   * @return Const reference to launch context
   *
   * @pre Service must be valid
   * @post Returns const reference to internal context
   *
   * @note Context lifetime tied to service
   *
   * @thread_safety Thread-safe for read-only access
   */
  const LaunchContext& get_context() const;
  
 private:
  /**
   * @brief Main event processing loop
   *
   * @details Processes events until shutdown requested.
   *          Called internally by Run().
   */
  void run_loop();

  /**
   * @brief Process a single event
   *
   * @details Handles one event from the event queue.
   *          Called internally by RunLoop().
   */
  void process_one_event();

  /**
   * @brief Setup signal handlers
   *
   * @details Configures signal handling for graceful shutdown.
   *          Called internally during initialization.
   */
  void setup_signal_handlers();

  /**
   * @brief Handle shutdown request
   *
   * @param reason Shutdown reason for logging
   *
   * @details Performs cleanup and state transitions for shutdown.
   */
  void handle_shutdown(const std::string& reason);
  
  /**
   * @brief Private implementation (Pimpl idiom)
   *
   * @details Hides implementation details for ABI compatibility.
   */
  class Impl;
  std::unique_ptr<Impl> impl_;
  
  /**
   * @brief Current service status
   *
   * @details Atomic status for thread-safe state queries.
   *          Use memory_order_acquire for loads, release for stores.
   */
  std::atomic<LaunchServiceStatus> status_;
  
  /**
   * @brief Shutdown requested flag
   *
   * @details Atomic flag for shutdown coordination.
   *          Set by Shutdown(), checked by RunLoop().
   */
  std::atomic<bool> shutdown_requested_;
  
  /**
   * @brief Registered launch descriptions
   *
   * @details Vector of launch descriptions to process.
   *          Protected by descriptions_mutex_.
   */
  std::vector<LaunchDescriptionPtr> descriptions_;
  
  /**
   * @brief Mutex for descriptions_ access
   *
   * @details Protects concurrent access to descriptions_ vector.
   *          Lock order: descriptions_mutex_ must be acquired before other locks.
   */
  std::mutex descriptions_mutex_;
  
  /**
   * @brief Worker thread for event processing
   *
   * @details Background thread for handling asynchronous events.
   *          Started during Run(), joined during Shutdown().
   */
  std::thread worker_thread_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__LAUNCH_SERVICE_HPP_
