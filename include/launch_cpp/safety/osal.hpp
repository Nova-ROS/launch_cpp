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
 * @file osal.hpp
 * @brief Operating System Abstraction Layer Interface
 *
 * @details This file defines the OSAL interfaces for ISO 26262 compliant
 *          process management. These interfaces abstract system calls to
 *          enable testing and portability while maintaining ASIL B safety level.
 *
 *          The OSAL provides:
 *          - Process execution and management
 *          - Resource monitoring
 *          - Watchdog functionality
 *          - Error handling
 *
 * @ASIL ASIL B
 *
 * @purpose Abstract operating system services for safety-critical process management
 *
 * @requirements
 * - REQ-LAUNCH-OSAL-001: Abstract process creation and control
 * - REQ-LAUNCH-OSAL-002: Provide resource monitoring capabilities
 * - REQ-LAUNCH-OSAL-003: Implement watchdog for health monitoring
 * - REQ-LAUNCH-OSAL-004: Enable mocking for unit testing
 */

#ifndef LAUNCH_CPP_OSAL_HPP_
#define LAUNCH_CPP_OSAL_HPP_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace launch_cpp {

// Forward declarations
class ProcessExecutor;
class Watchdog;
class ResourceMonitor;
class ErrorHandler;

/**
 * @brief Status codes for OSAL operations
 *
 * @details Enumerates possible outcomes of OSAL operations.
 *          Provides granularity beyond simple success/failure.
 *
 * @note Distinct from ErrorCode in error_code.hpp
 * @note Used with OsalResult for operation results
 *
 * @requirements REQ-LAUNCH-OSAL-001
 */
enum class OsalStatus {
    kSuccess,           ///< Operation completed successfully
    kError,             ///< General error occurred
    kTimeout,           ///< Operation timed out
    kResourceExhausted, ///< System resources exhausted
    kPermissionDenied,  ///< Insufficient permissions
    kNotFound,          ///< Resource not found
    kInvalidArgument    ///< Invalid parameter
};

/**
 * @brief Result type for OSAL operations (non-void types)
 *
 * @tparam T Type of successful result
 *
 * @details Similar to launch_cpp::Result but specific to OSAL
 *          operations. Provides error status alongside value.
 *
 * @note Check IsSuccess() before accessing value
 * @note Default constructs to success state
 *
 * @requirements REQ-LAUNCH-OSAL-001
 */
template<typename T>
class OsalResult {
public:
    /**
     * @brief Default constructor (success)
     */
    OsalResult() : status_(OsalStatus::kSuccess) {}
    
    /**
     * @brief Construct from value
     * @param value Value to store
     */
    explicit OsalResult(T value) : status_(OsalStatus::kSuccess), value_(std::move(value)) {}
    
    /**
     * @brief Construct from status
     * @param status Error status
     */
    explicit OsalResult(OsalStatus status) : status_(status) {}
    
    /**
     * @brief Construct from status with message
     * @param status Error status
     * @param message Error description
     */
    OsalResult(OsalStatus status, const std::string& message)
        : status_(status), error_message_(message) {}

    /**
     * @brief Check if operation succeeded
     * @return true if status is kSuccess
     */
    bool IsSuccess() const { return status_ == OsalStatus::kSuccess; }
    
    /**
     * @brief Check if operation failed
     * @return true if status is not kSuccess
     */
    bool HasError() const { return status_ != OsalStatus::kSuccess; }

    /**
     * @brief Get the status code
     * @return OsalStatus value
     */
    OsalStatus GetStatus() const { return status_; }
    
    /**
     * @brief Get error message
     * @return Error description (empty if success)
     */
    const std::string& GetErrorMessage() const { return error_message_; }

    /**
     * @brief Get value reference
     * @return Reference to stored value
     * @pre IsSuccess() must be true
     */
    T& GetValue() { return value_; }
    
    /**
     * @brief Get value const reference
     * @return Const reference to stored value
     * @pre IsSuccess() must be true
     */
    const T& GetValue() const { return value_; }

private:
    OsalStatus status_;      ///< Operation status
    T value_;                ///< Result value (valid if success)
    std::string error_message_;  ///< Error description
};

/**
 * @brief Specialization of OsalResult for void type
 *
 * @details For operations that succeed or fail without returning data.
 */
template<>
class OsalResult<void> {
public:
    /**
     * @brief Default constructor (success)
     */
    OsalResult() : status_(OsalStatus::kSuccess) {}
    
    /**
     * @brief Construct from status
     * @param status Error status
     */
    explicit OsalResult(OsalStatus status) : status_(status) {}
    
    /**
     * @brief Construct from status with message
     * @param status Error status
     * @param message Error description
     */
    OsalResult(OsalStatus status, const std::string& message)
        : status_(status), error_message_(message) {}

    /**
     * @brief Check if operation succeeded
     * @return true if status is kSuccess
     */
    bool IsSuccess() const { return status_ == OsalStatus::kSuccess; }
    
    /**
     * @brief Check if operation failed
     * @return true if status is not kSuccess
     */
    bool HasError() const { return status_ != OsalStatus::kSuccess; }

    /**
     * @brief Get the status code
     * @return OsalStatus value
     */
    OsalStatus GetStatus() const { return status_; }
    
    /**
     * @brief Get error message
     * @return Error description (empty if success)
     */
    const std::string& GetErrorMessage() const { return error_message_; }

private:
    OsalStatus status_;      ///< Operation status
    std::string error_message_;  ///< Error description
};

/**
 * @brief Process identifier type
 *
 * @note Negative values indicate invalid/unknown PID
 */
using ProcessId = int32_t;

/**
 * @brief Process state enumeration
 *
 * @details Tracks the lifecycle state of a managed process.
 *          Used for monitoring and health checking.
 *
 * @requirements REQ-LAUNCH-OSAL-001
 */
enum class ProcessState {
    kNotStarted,      ///< Process has not been started
    kStarting,        ///< Process is starting
    kRunning,         ///< Process is running
    kStopping,        ///< Process is being stopped
    kStopped,         ///< Process has stopped
    kCrashed,         ///< Process crashed
    kUnknown          ///< State unknown
};

/**
 * @brief Command line representation
 *
 * @details Encapsulates all parameters needed to execute a process.
 *          Provides conversion to argv format for exec system calls.
 *
 * @note All strings must outlive the ToArgv() call
 *
 * @requirements REQ-LAUNCH-OSAL-001
 */
struct CommandLine {
    std::string program;                    ///< Program to execute
    std::vector<std::string> arguments;     ///< Command arguments
    std::string working_directory;          ///< Working directory (empty = current)
    std::vector<std::pair<std::string, std::string>> environment; ///< Environment variables

    /**
     * @brief Convert to argv format for exec
     * @return Vector of char* (null-terminated)
     * @pre CommandLine object must outlive the returned pointers
     * @post Returns argv-compatible array
     * @note The returned pointers are valid as long as this CommandLine exists
     */
    std::vector<char*> ToArgv() const;
};

/**
 * @brief Process execution options
 *
 * @details Configuration for process execution including
 *          timeouts, restart policies, and output capture.
 *
 * @requirements REQ-LAUNCH-OSAL-001
 */
struct ProcessOptions {
    std::chrono::milliseconds startup_timeout{30000};  ///< Startup timeout
    std::chrono::milliseconds shutdown_timeout{5000};  ///< Graceful shutdown timeout
    bool auto_restart{false};                           ///< Auto-restart on crash
    uint32_t max_restarts{3};                          ///< Max restart attempts
    std::chrono::milliseconds restart_delay{10000};    ///< Delay between restarts
    bool capture_stdout{false};                        ///< Capture stdout
    bool capture_stderr{false};                        ///< Capture stderr
};

/**
 * @brief Process execution result
 *
 * @details Contains all information about a completed process execution.
 *          Returned by Wait() and related operations.
 *
 * @requirements REQ-LAUNCH-OSAL-001
 */
struct ProcessResult {
    ProcessId pid{-1};                    ///< Process ID
    int32_t exit_code{0};                 ///< Exit code (if exited)
    ProcessState final_state;             ///< Final process state
    std::string stdout_data;              ///< Captured stdout (if enabled)
    std::string stderr_data;              ///< Captured stderr (if enabled)
    std::chrono::steady_clock::time_point start_time;  ///< Start time
    std::chrono::steady_clock::time_point end_time;    ///< End time
};

/**
 * @brief Process executor interface
 *
 * @details Abstracts process creation and management operations.
 *          Enables mocking for unit testing and portability.
 *
 *          All methods are thread-safe unless noted otherwise.
 *
 * @ASIL ASIL B
 *
 * @requirements REQ-LAUNCH-OSAL-001, REQ-LAUNCH-OSAL-004
 */
class ProcessExecutor {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ProcessExecutor() = default;

    /**
     * @brief Execute a command
     * @param command Command to execute
     * @param options Execution options
     * @return Result containing process ID or error
     *
     * @pre command.program must not be empty
     * @post On success, returns valid ProcessId
     * @post On failure, returns error status
     *
     * @thread_safety Thread-safe
     *
     * @requirements REQ-LAUNCH-OSAL-001
     */
    virtual OsalResult<ProcessId> Execute(
        const CommandLine& command,
        const ProcessOptions& options) = 0;

    /**
     * @brief Wait for process to terminate
     * @param pid Process ID
     * @param timeout Maximum time to wait
     * @return Result containing exit information or error
     *
     * @pre pid must be valid
     * @post Returns when process exits or timeout occurs
     *
     * @thread_safety Thread-safe
     *
     * @requirements REQ-LAUNCH-OSAL-001
     */
    virtual OsalResult<ProcessResult> Wait(
        ProcessId pid,
        std::chrono::milliseconds timeout) = 0;

    /**
     * @brief Check if process is still running
     * @param pid Process ID
     * @return Result containing true if running, false otherwise
     *
     * @thread_safety Thread-safe
     *
     * @requirements REQ-LAUNCH-OSAL-001
     */
    virtual OsalResult<bool> IsRunning(ProcessId pid) = 0;

    /**
     * @brief Terminate a process gracefully
     * @param pid Process ID
     * @param timeout Time to wait for graceful termination
     * @return Success or error
     *
     * @pre pid must be valid
     * @post Sends SIGTERM, waits for timeout, then SIGKILL if needed
     *
     * @thread_safety Thread-safe
     *
     * @requirements REQ-LAUNCH-OSAL-001
     */
    virtual OsalResult<void> Terminate(
        ProcessId pid,
        std::chrono::milliseconds timeout) = 0;

    /**
     * @brief Kill a process immediately
     * @param pid Process ID
     * @return Success or error
     *
     * @pre pid must be valid
     * @post Sends SIGKILL
     *
     * @thread_safety Thread-safe
     *
     * @requirements REQ-LAUNCH-OSAL-001
     */
    virtual OsalResult<void> Kill(ProcessId pid) = 0;

    /**
     * @brief Send signal to process
     * @param pid Process ID
     * @param signal Signal number
     * @return Success or error
     *
     * @thread_safety Thread-safe
     *
     * @requirements REQ-LAUNCH-OSAL-001
     */
    virtual OsalResult<void> SendSignal(ProcessId pid, int32_t signal) = 0;

    /**
     * @brief Get process state
     * @param pid Process ID
     * @return Result containing process state
     *
     * @thread_safety Thread-safe
     *
     * @requirements REQ-LAUNCH-OSAL-001
     */
    virtual OsalResult<ProcessState> GetState(ProcessId pid) = 0;
};

/**
 * @brief Concrete implementation using POSIX system calls
 *
 * @details Production implementation using fork(), exec(), and signals.
 *          This is a thin adapter over POSIX APIs.
 *
 * @ASIL Simple adapter, code reviewed, minimal logic
 *
 * @requirements REQ-LAUNCH-OSAL-001
 */
class PosixProcessExecutor : public ProcessExecutor {
public:
    PosixProcessExecutor();
    ~PosixProcessExecutor() override;

    OsalResult<ProcessId> Execute(
        const CommandLine& command,
        const ProcessOptions& options) override;

    OsalResult<ProcessResult> Wait(
        ProcessId pid,
        std::chrono::milliseconds timeout) override;

    OsalResult<bool> IsRunning(ProcessId pid) override;

    OsalResult<void> Terminate(
        ProcessId pid,
        std::chrono::milliseconds timeout) override;

    OsalResult<void> Kill(ProcessId pid) override;

    OsalResult<void> SendSignal(ProcessId pid, int32_t signal) override;

    OsalResult<ProcessState> GetState(ProcessId pid) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Mock implementation for unit testing
 *
 * @details Allows injection of mock behavior for testing.
 *          NOT for production use.
 *
 * @ASIL Not used in production, for testing only
 *
 * @requirements REQ-LAUNCH-OSAL-004
 */
class MockProcessExecutor : public ProcessExecutor {
public:
    MockProcessExecutor();
    ~MockProcessExecutor() override;

    // Function types for mock callbacks
    using ExecuteCallback = std::function<OsalResult<ProcessId>(
        const CommandLine&, const ProcessOptions&)>;
    using WaitCallback = std::function<OsalResult<ProcessResult>(
        ProcessId, std::chrono::milliseconds)>;
    using IsRunningCallback = std::function<OsalResult<bool>(ProcessId)>;
    using TerminateCallback = std::function<OsalResult<void>(
        ProcessId, std::chrono::milliseconds)>;
    using KillCallback = std::function<OsalResult<void>(ProcessId)>;
    using SendSignalCallback = std::function<OsalResult<void>(ProcessId, int32_t)>;
    using GetStateCallback = std::function<OsalResult<ProcessState>(ProcessId)>;

    void SetExecuteCallback(ExecuteCallback callback);
    void SetWaitCallback(WaitCallback callback);
    void SetIsRunningCallback(IsRunningCallback callback);
    void SetTerminateCallback(TerminateCallback callback);
    void SetKillCallback(KillCallback callback);
    void SetSendSignalCallback(SendSignalCallback callback);
    void SetGetStateCallback(GetStateCallback callback);

    // Default implementations that can be overridden
    OsalResult<ProcessId> Execute(
        const CommandLine& command,
        const ProcessOptions& options) override;

    OsalResult<ProcessResult> Wait(
        ProcessId pid,
        std::chrono::milliseconds timeout) override;

    OsalResult<bool> IsRunning(ProcessId pid) override;

    OsalResult<void> Terminate(
        ProcessId pid,
        std::chrono::milliseconds timeout) override;

    OsalResult<void> Kill(ProcessId pid) override;

    OsalResult<void> SendSignal(ProcessId pid, int32_t signal) override;

    OsalResult<ProcessState> GetState(ProcessId pid) override;

private:
    ExecuteCallback execute_cb_;
    WaitCallback wait_cb_;
    IsRunningCallback is_running_cb_;
    TerminateCallback terminate_cb_;
    KillCallback kill_cb_;
    SendSignalCallback send_signal_cb_;
    GetStateCallback get_state_cb_;
};

/**
 * @brief Resource usage information
 *
 * @details Snapshot of resource consumption for a process.
 *
 * @requirements REQ-LAUNCH-OSAL-002
 */
struct ResourceUsage {
    uint64_t memory_bytes{0};           ///< Memory usage in bytes
    uint64_t virtual_memory_bytes{0};   ///< Virtual memory usage
    double cpu_percent{0.0};            ///< CPU usage percentage
    uint32_t file_descriptors{0};       ///< Number of open file descriptors
    uint32_t threads{0};                ///< Number of threads
};

/**
 * @brief System resource information
 *
 * @details System-wide resource availability and utilization.
 *
 * @requirements REQ-LAUNCH-OSAL-002
 */
struct SystemResources {
    uint64_t total_memory_bytes{0};
    uint64_t available_memory_bytes{0};
    uint64_t total_swap_bytes{0};
    uint64_t available_swap_bytes{0};
    double cpu_load_1min{0.0};
    double cpu_load_5min{0.0};
    double cpu_load_15min{0.0};
    uint32_t total_processes{0};
    uint32_t max_file_descriptors{0};
    uint32_t used_file_descriptors{0};
};

/**
 * @brief Resource monitor interface
 *
 * @details Monitors system and process resource usage.
 *          Provides alerts when thresholds are exceeded.
 *
 * @ASIL ASIL B
 *
 * @requirements REQ-LAUNCH-OSAL-002
 */
class ResourceMonitor {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ResourceMonitor() = default;

    /**
     * @brief Get current system resource usage
     * @return System resource information
     */
    virtual OsalResult<SystemResources> GetSystemResources() = 0;

    /**
     * @brief Get resource usage of a specific process
     * @param pid Process ID
     * @return Process resource usage
     */
    virtual OsalResult<ResourceUsage> GetProcessResources(ProcessId pid) = 0;

    /**
     * @brief Check if resources are available for new process
     * @param estimated_memory Estimated memory needed
     * @return true if resources available
     */
    virtual OsalResult<bool> AreResourcesAvailable(
        uint64_t estimated_memory) = 0;

    /**
     * @brief Set resource limits for a process
     * @param pid Process ID
     * @param max_memory Maximum memory allowed
     * @param max_cpu_percent Maximum CPU percentage
     * @return Success or error
     */
    virtual OsalResult<void> SetResourceLimits(
        ProcessId pid,
        uint64_t max_memory,
        double max_cpu_percent) = 0;

    /**
     * @brief Register a callback for resource threshold alerts
     * @param threshold Threshold percentage (0-100)
     * @param callback Callback function
     */
    virtual void RegisterThresholdCallback(
        double threshold,
        std::function<void(const SystemResources&)> callback) = 0;
};

/**
 * @brief Concrete resource monitor implementation
 *
 * @details POSIX-based resource monitoring using /proc filesystem.
 *
 * @requirements REQ-LAUNCH-OSAL-002
 */
class PosixResourceMonitor : public ResourceMonitor {
public:
    PosixResourceMonitor();
    ~PosixResourceMonitor() override;

    OsalResult<SystemResources> GetSystemResources() override;
    OsalResult<ResourceUsage> GetProcessResources(ProcessId pid) override;
    OsalResult<bool> AreResourcesAvailable(uint64_t estimated_memory) override;
    OsalResult<void> SetResourceLimits(
        ProcessId pid,
        uint64_t max_memory,
        double max_cpu_percent) override;
    void RegisterThresholdCallback(
        double threshold,
        std::function<void(const SystemResources&)> callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Heartbeat message structure
 *
 * @details Used by watchdog for health monitoring.
 *          Includes checksum for integrity verification.
 *
 * @requirements REQ-LAUNCH-OSAL-003
 */
struct HeartbeatMessage {
    uint32_t node_id{0};                    ///< Node identifier
    uint32_t sequence{0};                   ///< Sequence number
    uint64_t timestamp_us{0};               ///< Timestamp in microseconds
    ProcessState state{ProcessState::kUnknown}; ///< Current state
    uint32_t checksum{0};                   ///< Message checksum

    /**
     * @brief Calculate checksum
     * @return Calculated checksum
     */
    uint32_t CalculateChecksum() const {
        // Simple checksum: sum of all fields except checksum itself
        uint32_t sum = node_id + sequence + static_cast<uint32_t>(timestamp_us & 0xFFFFFFFF)
                      + static_cast<uint32_t>((timestamp_us >> 32) & 0xFFFFFFFF)
                      + static_cast<uint32_t>(state);
        return ~sum + 1;  // Two's complement for simple integrity check
    }
};

/**
 * @brief Heartbeat callback function type
 */
using HeartbeatCallback = std::function<void(const HeartbeatMessage&)>;

/**
 * @brief Watchdog interface
 *
 * @details Monitors node health via heartbeat mechanism.
 *          Detects unresponsive nodes and triggers recovery.
 *
 * @ASIL ASIL B
 *
 * @requirements REQ-LAUNCH-OSAL-003
 */
class Watchdog {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~Watchdog() = default;

    /**
     * @brief Register a node for monitoring
     * @param node_id Node identifier
     * @param timeout_ms Heartbeat timeout in milliseconds
     * @param callback Callback for heartbeat received
     * @return Success or error
     */
    virtual OsalResult<void> RegisterNode(
        uint32_t node_id,
        uint32_t timeout_ms,
        HeartbeatCallback callback) = 0;

    /**
     * @brief Unregister a node
     * @param node_id Node identifier
     * @return Success or error
     */
    virtual OsalResult<void> UnregisterNode(uint32_t node_id) = 0;

    /**
     * @brief Submit heartbeat for a node
     * @param message Heartbeat message
     * @return Success or error
     */
    virtual OsalResult<void> SubmitHeartbeat(
        const HeartbeatMessage& message) = 0;

    /**
     * @brief Check if node is responsive
     * @param node_id Node identifier
     * @return true if responsive
     */
    virtual OsalResult<bool> IsResponsive(uint32_t node_id) = 0;

    /**
     * @brief Set callback for node timeout
     * @param callback Callback function
     */
    virtual void SetTimeoutCallback(
        std::function<void(uint32_t)> callback) = 0;

    /**
     * @brief Start the watchdog
     * @return Success or error
     */
    virtual OsalResult<void> Start() = 0;

    /**
     * @brief Stop the watchdog
     * @return Success or error
     */
    virtual OsalResult<void> Stop() = 0;
};

/**
 * @brief Concrete watchdog implementation
 *
 * @details POSIX-based watchdog using threads and timers.
 *
 * @requirements REQ-LAUNCH-OSAL-003
 */
class PosixWatchdog : public Watchdog {
public:
    PosixWatchdog();
    ~PosixWatchdog() override;

    OsalResult<void> RegisterNode(
        uint32_t node_id,
        uint32_t timeout_ms,
        HeartbeatCallback callback) override;

    OsalResult<void> UnregisterNode(uint32_t node_id) override;

    OsalResult<void> SubmitHeartbeat(
        const HeartbeatMessage& message) override;

    OsalResult<bool> IsResponsive(uint32_t node_id) override;

    void SetTimeoutCallback(
        std::function<void(uint32_t)> callback) override;

    OsalResult<void> Start() override;
    OsalResult<void> Stop() override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Error severity levels
 *
 * @details Classification of error impact for handling decisions.
 *
 * @requirements REQ-LAUNCH-OSAL-005
 */
enum class ErrorSeverity {
    kInfo,       ///< Informational
    kWarning,    ///< Warning
    kError,      ///< Error (recoverable)
    kCritical,   ///< Critical (system impact)
    kFatal       ///< Fatal (safety impact)
};

/**
 * @brief Error information structure
 *
 * @details Comprehensive error information for logging and analysis.
 *
 * @requirements REQ-LAUNCH-OSAL-005
 */
struct ErrorInfo {
    uint64_t timestamp_us{0};           ///< Error timestamp
    ErrorSeverity severity;             ///< Error severity
    uint32_t error_code{0};             ///< Error code
    std::string error_message;          ///< Error message
    std::string context;                ///< Error context
    std::string file;                   ///< Source file
    uint32_t line{0};                   ///< Source line
    std::string function;               ///< Function name
};

/**
 * @brief Error handler interface
 *
 * @details Centralized error handling and reporting.
 *          Provides logging and notification for errors.
 *
 * @ASIL ASIL B
 *
 * @requirements REQ-LAUNCH-OSAL-005
 */
class ErrorHandler {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ErrorHandler() = default;

    /**
     * @brief Report an error
     * @param error Error information
     */
    virtual void ReportError(const ErrorInfo& error) = 0;

    /**
     * @brief Set callback for critical errors
     * @param callback Callback function
     */
    virtual void SetCriticalErrorCallback(
        std::function<void(const ErrorInfo&)> callback) = 0;

    /**
     * @brief Get last error
     * @return Last error information
     */
    virtual OsalResult<ErrorInfo> GetLastError() = 0;

    /**
     * @brief Clear error history
     */
    virtual void ClearErrors() = 0;

    /**
     * @brief Initialize error handler
     * @param log_path Path to error log
     * @return Success or error
     */
    virtual OsalResult<void> Initialize(
        const std::string& log_path) = 0;

    /**
     * @brief Shutdown error handler
     */
    virtual void Shutdown() = 0;
};

/**
 * @brief Concrete error handler implementation
 *
 * @details POSIX-based error logging to files.
 *
 * @requirements REQ-LAUNCH-OSAL-005
 */
class PosixErrorHandler : public ErrorHandler {
public:
    PosixErrorHandler();
    ~PosixErrorHandler() override;

    void ReportError(const ErrorInfo& error) override;
    void SetCriticalErrorCallback(
        std::function<void(const ErrorInfo&)> callback) override;
    OsalResult<ErrorInfo> GetLastError() override;
    void ClearErrors() override;
    OsalResult<void> Initialize(const std::string& log_path) override;
    void Shutdown() override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Convenience macros for error reporting
 *
 * @details These macros automatically populate ErrorInfo fields
 *          with current location information.
 *
 * @note Do not use these in headers - use only in .cpp files
 * @note These are convenience wrappers, not required
 *
 * @requirements REQ-LAUNCH-OSAL-005
 */
#define OSAL_REPORT_ERROR(handler, severity, code, message) \
    do { \
        launch_cpp::ErrorInfo error; \
        error.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>( \
            std::chrono::steady_clock::now().time_since_epoch()).count(); \
        error.severity = severity; \
        error.error_code = code; \
        error.error_message = message; \
        error.file = __FILE__; \
        error.line = __LINE__; \
        error.function = __func__; \
        handler->ReportError(error); \
    } while(0)

#define OSAL_REPORT_INFO(handler, code, message) \
    OSAL_REPORT_ERROR(handler, launch_cpp::ErrorSeverity::kInfo, code, message)

#define OSAL_REPORT_WARNING(handler, code, message) \
    OSAL_REPORT_ERROR(handler, launch_cpp::ErrorSeverity::kWarning, code, message)

#define OSAL_REPORT_CRITICAL(handler, code, message) \
    OSAL_REPORT_ERROR(handler, launch_cpp::ErrorSeverity::kCritical, code, message)

} // namespace launch_cpp

#endif // CPP_LAUNCH_OSAL_HPP_
