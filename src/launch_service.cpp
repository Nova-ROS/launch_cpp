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
 * @file launch_service.cpp
 * @brief LaunchService implementation
 *
 * @details Implements the main launch service that manages launch descriptions,
 *          executes actions, and handles lifecycle events. This is the core
 *          orchestration component of the launch system.
 *
 * @ASIL ASIL B
 *
 * @purpose Implement the main launch service orchestration
 *
 * @requirements
 * - REQ-LAUNCH-SERVICE-001: Manage launch description lifecycle
 * - REQ-LAUNCH-SERVICE-002: Execute actions in context
 * - REQ-LAUNCH-SERVICE-003: Handle graceful shutdown
 * - REQ-LAUNCH-SERVICE-004: Provide thread-safe state management
 */

#include "launch_cpp/launch_service.hpp"
#include "launch_context_impl.hpp"
#include "launch_cpp/launch_description.hpp"
#include "launch_cpp/event.hpp"
#include <iostream>

namespace launch_cpp
{

/**
 * @brief Private implementation of LaunchService
 *
 * @details Holds internal state and context to maintain ABI compatibility.
 *          This pattern allows changing internals without affecting users.
 */
class LaunchService::Impl
{
 public:
  /**
   * @brief Construct implementation with options
   *
   * @param options Service configuration options
   * @post Context initialized with provided arguments
   */
  explicit Impl(const Options& options)
    : context_(LaunchContext::Arguments{options.argv, options.noninteractive})
  {
    // Options reserved for future expansion
    (void)options;
  }

  /**
   * @brief Launch context implementation
   * @note Provides concrete implementation of abstract LaunchContext interface
   */
  LaunchContextImpl context_;
};

/**
 * @brief Construct launch service
 *
 * @param options Service configuration
 * @post Service created in idle state
 * @post Implementation allocated
 *
 * @note Service starts in kIdle state, transitions to kRunning via Run()
 * @note AUTOSAR C++14: Use member initialization list
 *
 * @thread_safety Not thread-safe during construction
 *
 * @requirements REQ-LAUNCH-SERVICE-001
 */
LaunchService::LaunchService(const Options& options)
  : impl_(std::make_unique<Impl>(options)),
    status_(LaunchServiceStatus::kIdle),
    shutdown_requested_(false)
{
}

/**
 * @brief Destroy launch service
 *
 * @pre Service may be in any state
 * @post Service shut down and resources released
 *
 * @note Automatically calls Shutdown() to ensure clean termination
 * @note noexcept - must not throw during destruction
 *
 * @safety_critical Yes - ensures clean shutdown on scope exit
 *
 * @requirements REQ-LAUNCH-SERVICE-003
 */
LaunchService::~LaunchService() noexcept
{
  // Ensure clean shutdown even if not explicitly called
  // Safety: Must not throw from destructor
  shutdown();
}

/**
 * @brief Run the launch service
 *
 * @details Main execution entry point. Transitions from idle to running,
 *          visits all registered launch descriptions, and optionally shuts down.
 *
 * @param shutdownWhenIdle If true, shut down after processing all descriptions
 * @return Exit code (0 = success, non-zero = error)
 *
 * @pre Service must be in kIdle state
 * @post If successful, service transitions through kRunning to kStopped
 * @post If shutdownWhenIdle, service is shut down before return
 *
 * @note Thread-safe state transition using compare_exchange
 * @note AUTOSAR C++14: Memory ordering for atomic operations
 *
 * @thread_safety Not reentrant - single execution at a time
 *
 * @safety_critical Yes - manages process lifecycle
 * @safety_note All error conditions logged before return
 *
 * @requirements REQ-LAUNCH-SERVICE-001, REQ-LAUNCH-SERVICE-002
 */
std::int32_t LaunchService::run(bool shutdown_when_idle)
{
  // Attempt state transition: kIdle -> kRunning
  // Using compare_exchange for thread-safe check-and-set
  LaunchServiceStatus expected = LaunchServiceStatus::kIdle;

  if (!status_.compare_exchange_strong(
        expected,
        LaunchServiceStatus::kRunning,
        std::memory_order_release,   // Success: synchronize-with readers
        std::memory_order_relaxed))  // Failure: no ordering needed
  {
    // State was not kIdle, cannot start
    std::cerr << "Launch service is not in idle state" << std::endl;
    return 1;
  }

  // Visit all registered launch descriptions
  // Copy vector under lock to avoid holding lock during execution
  std::vector<LaunchDescriptionPtr> descriptions;

  {
    // Critical section: Access shared descriptions vector
    // Safety: Minimize lock duration, copy vector contents
    std::lock_guard<std::mutex> lock(descriptions_mutex_);
    descriptions = descriptions_;
  }  // Lock released here

  // Process each launch description
  for (const auto& desc : descriptions)
  {
    // Safety: Skip null descriptions
    if (!desc)
    {
      continue;
    }

    // Visit description in context - this executes all contained actions
    // Requirements: REQ-LAUNCH-SERVICE-002
    Result<LaunchDescriptionEntityVector> result = desc->visit(impl_->context_);

    // Handle execution errors
    if (result.has_error())
    {
      std::cerr << "Error visiting launch description: "
                << result.get_error().get_message() << std::endl;
      return 1;
    }
  }

  // Optionally shut down after processing
  if (shutdown_when_idle)
  {
    shutdown();
  }

  return 0;
}

/**
 * @brief Add a launch description to the service
 *
 * @param description Launch description to include
 * @return Error object (success or error details)
 *
 * @pre description must not be nullptr
 * @post Description registered for execution
 *
 * @note Thread-safe via mutex protection
 * @note Descriptions are executed in registration order
 *
 * @thread_safety Thread-safe
 *
 * @requirements REQ-LAUNCH-SERVICE-001
 */
Error LaunchService::include_launch_description(const LaunchDescriptionPtr& description)
{
  // Validate input
  if (!description)
  {
    return Error(ErrorCode::kInvalidArgument, "Null launch description");
  }

  // Critical section: Modify shared descriptions vector
  // Safety: RAII lock guard ensures exception safety
  std::lock_guard<std::mutex> lock(descriptions_mutex_);
  descriptions_.push_back(description);

  return Error();  // Success
}

/**
 * @brief Emit an event to registered handlers
 *
 * @param event Event to emit
 *
 * @pre None
 * @post Event dispatched (when implemented)
 *
 * @note Currently not implemented - placeholder for future event system
 * @todo Implement event dispatch to registered handlers
 */
void LaunchService::emit_event(EventPtr event)
{
  (void)event;
  // TODO: Implement event handling
  // Requirements: Future REQ-LAUNCH-SERVICE-005
}

/**
 * @brief Shut down the launch service
 *
 * @details Initiates graceful shutdown, signals all components to stop,
 *          and waits for completion.
 *
 * @return Error object (success or error details)
 *
 * @pre May be called from any state
 * @post Service transitions to kStopped
 * @post shutdown_requested_ flag set
 *
 * @note Thread-safe state transition
 * @note Idempotent - safe to call multiple times
 *
 * @thread_safety Thread-safe
 *
 * @safety_critical Yes - ensures clean termination
 * @safety_note Must complete even if components are unresponsive
 *
 * @requirements REQ-LAUNCH-SERVICE-003
 */
Error LaunchService::shutdown()
{
  // Attempt state transition: kRunning -> kShuttingDown
  LaunchServiceStatus expected = LaunchServiceStatus::kRunning;

  if (!status_.compare_exchange_strong(
        expected,
        LaunchServiceStatus::kShuttingDown,
        std::memory_order_release,
        std::memory_order_relaxed))
  {
    // Already shutting down or stopped - this is OK
    return Error();
  }

  // Signal shutdown to all components
  shutdown_requested_.store(true, std::memory_order_release);

  // TODO: Wait for components to shut down gracefully
  // TODO: Timeout handling for unresponsive components

  // Final state transition
  status_.store(LaunchServiceStatus::kStopped, std::memory_order_release);

  return Error();  // Success
}

/**
 * @brief Check if service is running
 *
 * @return true if in kRunning state
 *
 * @pre None
 * @post Returns current state
 *
 * @note Thread-safe via atomic load
 * @note State may change immediately after return
 *
 * @thread_safety Thread-safe
 */
bool LaunchService::is_running() const noexcept
{
  return status_.load(std::memory_order_acquire) == LaunchServiceStatus::kRunning;
}

/**
 * @brief Check if service is idle
 *
 * @return true if in kIdle state
 *
 * @pre None
 * @post Returns current state
 *
 * @note Thread-safe via atomic load
 *
 * @thread_safety Thread-safe
 */
bool LaunchService::is_idle() const
{
  return status_.load(std::memory_order_acquire) == LaunchServiceStatus::kIdle;
}

/**
 * @brief Get the launch context (non-const)
 *
 * @return Reference to launch context
 *
 * @pre Service must be valid
 * @post Returns reference to internal context
 *
 * @note Context lifetime tied to service lifetime
 *
 * @thread_safety Not thread-safe with context modification
 */
LaunchContext& LaunchService::get_context()
{
  return impl_->context_;
}

/**
 * @brief Get the launch context (const)
 *
 * @return Const reference to launch context
 *
 * @pre Service must be valid
 * @post Returns const reference to internal context
 *
 * @note Context lifetime tied to service lifetime
 *
 * @thread_safety Thread-safe for read-only access
 */
const LaunchContext& LaunchService::get_context() const
{
  return impl_->context_;
}

}  // namespace launch_cpp
