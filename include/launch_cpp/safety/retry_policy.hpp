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

#ifndef LAUNCH_CPP_RETRY_POLICY_HPP_
#define LAUNCH_CPP_RETRY_POLICY_HPP_

#include <cstdint>
#include <chrono>
#include <set>
#include <functional>
#include <algorithm>
#include <thread>
#include <string>

namespace launch_cpp {

/**
 * @brief Error codes that can be retried
 */
enum class ErrorCode : uint32_t {
    kSuccess = 0,
    kForkFailed = 1,
    kExecFailed = 2,
    kTimeout = 3,
    kResourceExhausted = 4,
    kPermissionDenied = 5,
    kConfigInvalid = 6,
    kMaxRetriesExceeded = 7,
    kUnknown = 999
};

/**
 * @brief Retry configuration
 */
struct RetryConfig {
    uint32_t max_attempts{3};                           ///< Maximum retry attempts
    std::chrono::milliseconds initial_delay{5000};     ///< Initial delay between retries
    double backoff_multiplier{1.0};                     ///< Multiplier for exponential backoff (1.0 = linear)
    std::set<ErrorCode> retryable_errors;              ///< Set of error codes that should trigger retry

    RetryConfig() {
        // Default retryable errors
        retryable_errors = {
            ErrorCode::kForkFailed,
            ErrorCode::kResourceExhausted,
            ErrorCode::kTimeout
        };
    }

    /**
     * @brief Check if an error code is retryable
     */
    bool is_retryable(ErrorCode code) const {
        return retryable_errors.find(code) != retryable_errors.end();
    }
};

/**
 * @brief Result of an operation that can be retried
 */
template<typename T>
class RetryableResult {
public:
    RetryableResult() : success_(true), error_code_(ErrorCode::kSuccess) {}
    explicit RetryableResult(T value)
        : success_(true), error_code_(ErrorCode::kSuccess), value_(std::move(value)) {}
    explicit RetryableResult(ErrorCode code, const std::string& message = "")
        : success_(false), error_code_(code), error_message_(message) {}

    bool is_success() const { return success_; }
    bool has_error() const { return !success_; }
    ErrorCode get_error_code() const { return error_code_; }
    const std::string& get_error_message() const { return error_message_; }

    T& get_value() { return value_; }
    const T& get_value() const { return value_; }

private:
    bool success_;
    ErrorCode error_code_;
    std::string error_message_;
    T value_;
};

/**
 * @brief Retry policy implementation
 *
 * Implements configurable retry logic with:
 * - Maximum attempt limit
 * - Exponential or linear backoff
 * - Selective retry based on error type
 * - Timeout support
 *
 * @requirement TSR-004: Retry mechanism
 */
class RetryPolicy {
public:
    /**
     * @brief Construct with configuration
     */
    explicit RetryPolicy(const RetryConfig& config) : config_(config) {}

    /**
     * @brief Default constructor with default config
     */
    RetryPolicy() = default;

    ~RetryPolicy() = default;

    // Non-copyable (config can be large)
    RetryPolicy(const RetryPolicy&) = delete;
    RetryPolicy& operator=(const RetryPolicy&) = delete;

    // Movable
    RetryPolicy(RetryPolicy&&) = default;
    RetryPolicy& operator=(RetryPolicy&&) = default;

    /**
     * @brief Execute an operation with retry logic
     * @param operation Function to execute (returns RetryableResult<T>)
     * @return Final result (success or last error)
     *
     * Example:
     * @code
     * RetryPolicy policy(config);
     * auto result = policy.Execute<ProcessId>([&]() -> RetryableResult<ProcessId> {
     *     return TryStartProcess();
     * });
     * @endcode
     */
    template<typename T>
    RetryableResult<T> execute(std::function<RetryableResult<T>()> operation) const {
        uint32_t attempt = 0;
        RetryableResult<T> last_result;

        while (attempt < config_.max_attempts) {
            // Execute operation
            last_result = operation();

            // Check if successful
            if (last_result.is_success()) {
                return last_result;
            }

            // Check if error is retryable
            if (!should_retry(last_result.get_error_code())) {
                return last_result;
            }

            // Increment attempt counter
            attempt++;

            // If we've reached max attempts, return last error
            if (attempt >= config_.max_attempts) {
                break;
            }

            // Calculate and apply delay
            auto delay = calculate_delay(attempt);
            sleep(delay);
        }

        // Max retries exceeded
        return RetryableResult<T>(
            ErrorCode::kMaxRetriesExceeded,
            "Max retry attempts (" + std::to_string(config_.max_attempts) + ") exceeded"
        );
    }

    /**
     * @brief Check if an error should trigger retry
     * @param error_code Error code to check
     * @return true if error is retryable
     */
    bool should_retry(ErrorCode error_code) const {
        return config_.is_retryable(error_code);
    }

    /**
     * @brief Calculate delay for a specific attempt
     * @param attempt_number Current attempt (1-based)
     * @return Delay duration
     *
     * Formula: delay = initial_delay * (backoff_multiplier ^ (attempt - 1))
     *
     * Example with initial_delay=1000ms, multiplier=1.5:
     * - Attempt 1: 1000ms
     * - Attempt 2: 1500ms
     * - Attempt 3: 2250ms
     */
    std::chrono::milliseconds calculate_delay(uint32_t attempt_number) const {
        if (attempt_number == 0) {
            return std::chrono::milliseconds(0);
        }

        double multiplier = 1.0;
        for (uint32_t i = 1; i < attempt_number; ++i) {
            multiplier *= config_.backoff_multiplier;
        }

        auto delay_ms = static_cast<uint32_t>(
            config_.initial_delay.count() * multiplier
        );

        return std::chrono::milliseconds(delay_ms);
    }

    /**
     * @brief Get current configuration
     */
    const RetryConfig& get_config() const { return config_; }

    /**
     * @brief Get max attempts
     */
    uint32_t get_max_attempts() const { return config_.max_attempts; }

    /**
     * @brief Get initial delay
     */
    std::chrono::milliseconds get_initial_delay() const {
        return config_.initial_delay;
    }

private:
    RetryConfig config_;

    /**
     * @brief Sleep for specified duration
     * @param duration Sleep duration
     *
     * @note Uses std::this_thread::sleep_for
     * @note Can be mocked in tests
     */
    void sleep(std::chrono::milliseconds duration) const {
        std::this_thread::sleep_for(duration);
    }
};

// ============================================================================
// Non-retry policy (for testing and scenarios where retry is not desired)
// ============================================================================

/**
 * @brief No-retry policy - fails immediately on error
 */
class NoRetryPolicy {
public:
    template<typename T>
    RetryableResult<T> execute(std::function<RetryableResult<T>()> operation) const {
        return operation();
    }

    bool should_retry(ErrorCode) const { return false; }

    std::chrono::milliseconds calculate_delay(uint32_t) const {
        return std::chrono::milliseconds(0);
    }
};

// ============================================================================
// Fixed retry policy (for testing - always retries N times)
// ============================================================================

/**
 * @brief Fixed retry policy - always retries N times regardless of error
 */
class FixedRetryPolicy {
public:
    explicit FixedRetryPolicy(uint32_t max_attempts,
                               std::chrono::milliseconds delay)
        : max_attempts_(max_attempts), delay_(delay) {}

    template<typename T>
    RetryableResult<T> execute(std::function<RetryableResult<T>()> operation) const {
        RetryableResult<T> last_result;

        for (uint32_t i = 0; i < max_attempts_; ++i) {
            last_result = operation();
            if (last_result.is_success()) {
                return last_result;
            }
            if (i < max_attempts_ - 1) {
                std::this_thread::sleep_for(delay_);
            }
        }

        return last_result;
    }

    bool should_retry(ErrorCode) const { return true; }

    std::chrono::milliseconds calculate_delay(uint32_t) const { return delay_; }

private:
    uint32_t max_attempts_;
    std::chrono::milliseconds delay_;
};

// ============================================================================
// Test Support
// ============================================================================

#ifdef BUILD_TESTING
/**
 * @brief Test fixture for RetryPolicy tests
 */
class RetryPolicyTestFixture {
public:
    RetryPolicy create_default_policy() const {
        return RetryPolicy();
    }

    RetryPolicy create_exponential_backoff_policy() const {
        RetryConfig config;
        config.max_attempts = 5;
        config.initial_delay = std::chrono::milliseconds(100);
        config.backoff_multiplier = 2.0;  // Exponential
        return RetryPolicy(config);
    }

    RetryPolicy create_linear_policy() const {
        RetryConfig config;
        config.max_attempts = 3;
        config.initial_delay = std::chrono::milliseconds(1000);
        config.backoff_multiplier = 1.0;  // Linear
        return RetryPolicy(config);
    }

    RetryPolicy create_no_retry_policy() const {
        RetryConfig config;
        config.max_attempts = 1;
        return RetryPolicy(config);
    }

    // Helper to create a failing operation that succeeds after N attempts
    std::function<RetryableResult<int>()> create_fail_n_times_operation(
        uint32_t fail_count,
        ErrorCode error_code = ErrorCode::kTimeout) const {

        auto counter = std::make_shared<uint32_t>(0);
        return [counter, fail_count, error_code]() -> RetryableResult<int> {
            if (*counter < fail_count) {
                (*counter)++;
                return RetryableResult<int>(error_code, "Simulated failure");
            }
            return RetryableResult<int>(42);  // Success
        };
    }

    // Helper to create an always failing operation
    std::function<RetryableResult<int>()> create_always_fail_operation(
        ErrorCode error_code = ErrorCode::kTimeout) const {

        return [error_code]() -> RetryableResult<int> {
            return RetryableResult<int>(error_code, "Always fails");
        };
    }

    // Helper to create an always succeeding operation
    std::function<RetryableResult<int>()> create_always_succeed_operation(
        int value = 42) const {

        return [value]() -> RetryableResult<int> {
            return RetryableResult<int>(value);
        };
    }
};
#endif  // BUILD_TESTING

} // namespace launch_cpp

#endif // CPP_LAUNCH_RETRY_POLICY_HPP_
