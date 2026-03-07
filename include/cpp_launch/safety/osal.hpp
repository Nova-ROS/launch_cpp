/**
 * @file osal.hpp
 * @brief Operating System Abstraction Layer Interface
 * 
 * This file defines the OSAL interfaces for ISO 26262 compliant
 * process management. These interfaces abstract system calls to
 * enable testing and portability.
 * 
 * @copyright Copyright 2025
 * @license Apache License 2.0
 */

#ifndef CPP_LAUNCH_OSAL_HPP_
#define CPP_LAUNCH_OSAL_HPP_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace ara {
namespace exec {

// Forward declarations
class ProcessExecutor;
class Watchdog;
class ResourceMonitor;
class ErrorHandler;

/**
 * @brief Status codes for OSAL operations
 */
enum class OsalStatus {
    kSuccess,
    kError,
    kTimeout,
    kResourceExhausted,
    kPermissionDenied,
    kNotFound,
    kInvalidArgument
};

/**
 * @brief Result type for OSAL operations (non-void types)
 * 
 * Similar to cpp_launch::Result but specific to OSAL
 */
template<typename T>
class OsalResult {
public:
    OsalResult() : status_(OsalStatus::kSuccess) {}
    explicit OsalResult(T value) : status_(OsalStatus::kSuccess), value_(std::move(value)) {}
    explicit OsalResult(OsalStatus status) : status_(status) {}
    OsalResult(OsalStatus status, const std::string& message) 
        : status_(status), error_message_(message) {}

    bool IsSuccess() const { return status_ == OsalStatus::kSuccess; }
    bool HasError() const { return status_ != OsalStatus::kSuccess; }
    
    OsalStatus GetStatus() const { return status_; }
    const std::string& GetErrorMessage() const { return error_message_; }
    
    T& GetValue() { return value_; }
    const T& GetValue() const { return value_; }

private:
    OsalStatus status_;
    T value_;
    std::string error_message_;
};

/**
 * @brief Specialization of OsalResult for void type
 */
template<>
class OsalResult<void> {
public:
    OsalResult() : status_(OsalStatus::kSuccess) {}
    explicit OsalResult(OsalStatus status) : status_(status) {}
    OsalResult(OsalStatus status, const std::string& message) 
        : status_(status), error_message_(message) {}

    bool IsSuccess() const { return status_ == OsalStatus::kSuccess; }
    bool HasError() const { return status_ != OsalStatus::kSuccess; }
    
    OsalStatus GetStatus() const { return status_; }
    const std::string& GetErrorMessage() const { return error_message_; }

private:
    OsalStatus status_;
    std::string error_message_;
};

/**
 * @brief Process identifier type
 */
using ProcessId = int32_t;

/**
 * @brief Process state enumeration
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
 */
struct CommandLine {
    std::string program;                    ///< Program to execute
    std::vector<std::string> arguments;     ///< Command arguments
    std::string working_directory;          ///< Working directory (empty = current)
    std::vector<std::pair<std::string, std::string>> environment; ///< Environment variables
    
    /**
     * @brief Convert to argv format for exec
     * @return Vector of char* (null-terminated)
     * @note The returned pointers are valid as long as this CommandLine exists
     */
    std::vector<char*> ToArgv() const;
};

/**
 * @brief Process execution options
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
 * Abstracts process creation and management operations.
 * Enables mocking for unit testing.
 */
class ProcessExecutor {
public:
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
     */
    virtual OsalResult<void> Kill(ProcessId pid) = 0;

    /**
     * @brief Send signal to process
     * @param pid Process ID
     * @param signal Signal number
     * @return Success or error
     * 
     * @thread_safety Thread-safe
     */
    virtual OsalResult<void> SendSignal(ProcessId pid, int32_t signal) = 0;

    /**
     * @brief Get process state
     * @param pid Process ID
     * @return Result containing process state
     * 
     * @thread_safety Thread-safe
     */
    virtual OsalResult<ProcessState> GetState(ProcessId pid) = 0;
};

/**
 * @brief Concrete implementation using POSIX system calls
 * 
 * This is the production implementation.
 * ASIL: Simple adapter, code reviewed, minimal logic
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
 * Allows injection of mock behavior for testing.
 * ASIL: Not used in production, for testing only
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
 * Monitors system and process resource usage.
 */
class ResourceMonitor {
public:
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
 * Monitors node health via heartbeat mechanism.
 */
class Watchdog {
public:
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
 * Centralized error handling and reporting.
 */
class ErrorHandler {
public:
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
 */
#define OSAL_REPORT_ERROR(handler, severity, code, message) \
    do { \
        ara::exec::ErrorInfo error; \
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
    OSAL_REPORT_ERROR(handler, ara::exec::ErrorSeverity::kInfo, code, message)

#define OSAL_REPORT_WARNING(handler, code, message) \
    OSAL_REPORT_ERROR(handler, ara::exec::ErrorSeverity::kWarning, code, message)

#define OSAL_REPORT_CRITICAL(handler, code, message) \
    OSAL_REPORT_ERROR(handler, ara::exec::ErrorSeverity::kCritical, code, message)

} // namespace exec
} // namespace ara

#endif // CPP_LAUNCH_OSAL_HPP_
