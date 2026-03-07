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

#ifndef LAUNCH_CPP_NODE_ACTION_REFACTORED_HPP_
#define LAUNCH_CPP_NODE_ACTION_REFACTORED_HPP_

#include <memory>
#include <string>
#include <atomic>
#include "launch_cpp/action.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/result.hpp"
#include "../src/osal.hpp"
#include "command_builder.hpp"
#include "dependency_resolver.hpp"
#include "retry_policy.hpp"

namespace launch_cpp {

// Forward declarations
class NodeActionLogic;

/**
 * @brief Node states for state machine
 */
enum class NodeState {
    kUninitialized,     ///< Not yet initialized
    kInitialized,       ///< Initialized but not started
    kStarting,          ///< Starting process
    kRunning,           ///< Process running
    kNotResponding,     ///< Heartbeat timeout
    kStartFailed,       ///< Failed to start (terminal)
    kRestarting,        ///< Attempting restart
    kStopping,          ///< Stopping process
    kStopped,           ///< Stopped (terminal)
    kCrashed            ///< Crashed (terminal)
};

/**
 * @brief Node statistics
 */
struct NodeStatistics {
    uint32_t start_attempts{0};                         ///< Number of start attempts
    uint32_t restart_count{0};                          ///< Number of restarts
    uint32_t heartbeat_failures{0};                     ///< Heartbeat failure count
    std::chrono::steady_clock::time_point start_time;   ///< Last start time
    std::chrono::steady_clock::time_point stop_time;    ///< Last stop time
    std::chrono::milliseconds total_runtime{0};         ///< Total runtime
};

/**
 * @brief Refactored NodeAction with dependency injection
 *
 * This class implements the Action interface and manages a ROS2 node lifecycle.
 * It uses dependency injection to accept OSAL components, enabling:
 * - Unit testing with mock implementations
 * - Configuration of different behaviors
 * - Separation of concerns
 *
 * @requirement TSR-001 ~ TSR-005: Node lifecycle management
 */
class NodeActionRefactored : public Action {
public:
    /**
     * @brief Construct with dependencies
     *
     * @param options Node configuration
     * @param executor Process executor (OSAL)
     * @param watchdog Watchdog for monitoring
     * @param error_handler Error handler
     * @param retry_policy Retry policy
     *
     * All dependencies are injected and can be mocked for testing.
     */
    NodeActionRefactored(
        const NodeActionOptions& options,
        std::shared_ptr<launch_cpp::ProcessExecutor> executor,
        std::shared_ptr<launch_cpp::Watchdog> watchdog,
        std::shared_ptr<launch_cpp::ErrorHandler> error_handler,
        std::shared_ptr<RetryPolicy> retry_policy);

    /**
     * @brief Destructor
     */
    ~NodeActionRefactored() override;

    // Non-copyable
    NodeActionRefactored(const NodeActionRefactored&) = delete;
    NodeActionRefactored& operator=(const NodeActionRefactored&) = delete;

    // Non-movable (contains state)
    NodeActionRefactored(NodeActionRefactored&&) = delete;
    NodeActionRefactored& operator=(NodeActionRefactored&&) = delete;

    /**
     * @brief Execute the node action
     * @param context Launch context
     * @return Result of execution
     *
     * This is the main entry point. It:
     * 1. Validates configuration
     * 2. Builds command line
     * 3. Starts process with retry logic
     * 4. Registers with watchdog
     * 5. Returns result
     *
     * @requirement TSR-001: Configuration validation
     * @requirement TSR-003: Startup failure detection
     * @requirement TSR-004: Retry mechanism
     * @requirement TSR-005: Timeout handling
     */
    Result<void> Execute(LaunchContext& context) override;

    /**
     * @brief Stop the node
     * @return Success or error
     *
     * Implements graceful shutdown:
     * 1. Unregister from watchdog
     * 2. Send SIGTERM
     * 3. Wait for graceful exit
     * 4. Send SIGKILL if needed
     */
    Result<void> Stop();

    /**
     * @brief Get current node state
     */
    NodeState GetState() const { return state_.load(); }

    /**
     * @brief Get node name
     */
    const std::string& GetName() const { return options_.name; }

    /**
     * @brief Get package name
     */
    const std::string& GetPackage() const { return options_.package; }

    /**
     * @brief Get executable name
     */
    const std::string& GetExecutable() const { return options_.executable; }

    /**
     * @brief Get process ID (if running)
     * @return Process ID or -1 if not running
     */
    launch_cpp::ProcessId GetProcessId() const { return current_pid_.load(); }

    /**
     * @brief Get node statistics
     */
    NodeStatistics GetStatistics() const { return stats_; }

    /**
     * @brief Check if node is running
     */
    bool IsRunning() const {
        return state_.load() == NodeState::kRunning;
    }

    /**
     * @brief Handle heartbeat from node
     * @param message Heartbeat message
     *
     * Called by watchdog when heartbeat received.
     */
    void OnHeartbeat(const launch_cpp::HeartbeatMessage& message);

    /**
     * @brief Handle heartbeat timeout
     *
     * Called by watchdog when heartbeat timeout occurs.
     * Implements recovery logic per TSR-009.
     */
    void OnHeartbeatTimeout();

    /**
     * @brief Handle process exit
     * @param exit_code Process exit code
     * @param state Process state
     *
     * Called when process exits (normally or abnormally).
     */
    void OnProcessExit(int32_t exit_code, launch_cpp::ProcessState state);

private:
    // Configuration
    NodeActionOptions options_;

    // Injected dependencies (OSAL)
    std::shared_ptr<launch_cpp::ProcessExecutor> executor_;
    std::shared_ptr<launch_cpp::Watchdog> watchdog_;
    std::shared_ptr<launch_cpp::ErrorHandler> error_handler_;
    std::shared_ptr<RetryPolicy> retry_policy_;

    // Business logic (extracted)
    std::unique_ptr<CommandBuilder> command_builder_;

    // State
    std::atomic<NodeState> state_{NodeState::kUninitialized};
    std::atomic<launch_cpp::ProcessId> current_pid_{-1};

    // Statistics
    NodeStatistics stats_;
    mutable std::mutex stats_mutex_;

    // Constants
    static constexpr std::chrono::milliseconds kDefaultStartupTimeout{30000};
    static constexpr std::chrono::milliseconds kDefaultShutdownTimeout{5000};

    /**
     * @brief Execute with retry logic
     */
    Result<void> ExecuteWithRetry(LaunchContext& context);

    /**
     * @brief Single attempt to start node
     */
    Result<void> AttemptStart(LaunchContext& context);

    /**
     * @brief Register node with watchdog
     */
    Result<void> RegisterWithWatchdog();

    /**
     * @brief Unregister node from watchdog
     */
    void UnregisterFromWatchdog();

    /**
     * @brief Update node state
     */
    void UpdateState(NodeState new_state);

    /**
     * @brief Report error through error handler
     */
    void ReportError(ErrorCode code, const std::string& message);

    /**
     * @brief Calculate startup timeout
     */
    std::chrono::milliseconds GetStartupTimeout() const;

    /**
     * @brief Calculate shutdown timeout
     */
    std::chrono::milliseconds GetShutdownTimeout() const;

    /**
     * @brief Check if restart is allowed
     */
    bool IsRestartAllowed() const;

    /**
     * @brief Increment restart count
     */
    void IncrementRestartCount();

    /**
     * @brief Reset restart count
     */
    void ResetRestartCount();

    /**
     * @brief Update runtime statistics
     */
    void UpdateRuntimeStats();
};

// ============================================================================
// Inline Implementation - Simple Methods
// ============================================================================

inline void NodeActionRefactored::UpdateState(NodeState new_state) {
    NodeState old_state = state_.exchange(new_state);

    // Log state transition
    if (error_handler_) {
        // Only log significant transitions
        if (old_state != new_state) {
            // State changed - could log here
        }
    }
}

inline void NodeActionRefactored::ReportError(ErrorCode code,
                                               const std::string& message) {
    if (error_handler_) {
        launch_cpp::ErrorInfo error;
        error.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        error.error_code = static_cast<uint32_t>(code);
        error.error_message = message;
        error.context = "NodeAction: " + options_.name;
        // file, line, function would be set by macro

        error_handler_->ReportError(error);
    }
}

inline std::chrono::milliseconds NodeActionRefactored::GetStartupTimeout() const {
    // Could be configurable per node
    return kDefaultStartupTimeout;
}

inline std::chrono::milliseconds NodeActionRefactored::GetShutdownTimeout() const {
    // Could be configurable per node
    return kDefaultShutdownTimeout;
}

inline bool NodeActionRefactored::IsRestartAllowed() const {
    if (!retry_policy_) {
        return false;
    }

    return stats_.restart_count < retry_policy_->GetMaxAttempts();
}

inline void NodeActionRefactored::IncrementRestartCount() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.restart_count++;
}

inline void NodeActionRefactored::ResetRestartCount() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.restart_count = 0;
}

inline void NodeActionRefactored::UpdateRuntimeStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);

    if (stats_.start_time.time_since_epoch().count() > 0) {
        auto now = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - stats_.start_time);
        stats_.total_runtime += runtime;
    }
}

// ============================================================================
// Test Support
// ============================================================================

#ifdef BUILD_TESTING
/**
 * @brief Test fixture for NodeActionRefactored tests
 */
class NodeActionRefactoredTestFixture {
public:
    // Create mock dependencies
    struct MockDependencies {
        std::shared_ptr<launch_cpp::ProcessExecutor> executor;
        std::shared_ptr<launch_cpp::Watchdog> watchdog;
        std::shared_ptr<launch_cpp::ErrorHandler> error_handler;
        std::shared_ptr<RetryPolicy> retry_policy;
    };

    MockDependencies CreateMockDependencies() const {
        MockDependencies deps;
        // In real implementation, these would be Google Mock objects
        // deps.executor = std::make_shared<MockProcessExecutor>();
        // deps.watchdog = std::make_shared<MockWatchdog>();
        // etc.
        return deps;
    }

    NodeActionOptions CreateValidOptions() const {
        NodeActionOptions options;
        options.package = "demo_nodes_cpp";
        options.executable = "talker";
        options.name = "test_talker";
        options.namespace_ = "/test";
        return options;
    }

    std::shared_ptr<NodeActionRefactored> CreateNodeAction(
        const NodeActionOptions& options,
        const MockDependencies& deps) const {
        return std::make_shared<NodeActionRefactored>(
            options,
            deps.executor,
            deps.watchdog,
            deps.error_handler,
            deps.retry_policy
        );
    }
};
#endif  // BUILD_TESTING

} // namespace launch_cpp

#endif // CPP_LAUNCH_NODE_ACTION_REFACTORED_HPP_
