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
 * @file error_code.hpp
 * @brief Error handling types and Result pattern implementation
 *
 * @details This header provides error code enumerations, the Error class,
 *          and the Result monad for error handling without exceptions.
 *          Compliant with AUTOSAR C++14 prohibition on exceptions.
 *
 * @ASIL ASIL B
 *
 * @purpose Provide type-safe error handling without exceptions
 *
 * @requirements
 * - REQ-LAUNCH-ERROR-001: Define error codes for all failure modes
 * - REQ-LAUNCH-ERROR-002: Provide error information without exceptions
 * - REQ-LAUNCH-ERROR-003: Support Result pattern for error propagation
 */

#ifndef LAUNCH_CPP__ERROR_CODE_HPP_
#define LAUNCH_CPP__ERROR_CODE_HPP_

#include <cstdint>
#include <string>
#include <utility>

namespace launch_cpp
{

/**
 * @brief Error codes for launch_cpp operations
 *
 * @details Enumerates all possible error conditions in the launch system.
 *          Used with the Error class and Result type for error handling.
 *
 * @note AUTOSAR C++14: A7-2-4 - Use enum class for strong typing
 * @note Values are stable and can be used for programmatic error handling
 *
 * @requirements REQ-LAUNCH-ERROR-001
 */
enum class ErrorCode : std::int32_t
{
  kSuccess = 0,              ///< Operation completed successfully
  kInvalidArgument = 1,      ///< Invalid parameter passed to function
  kInvalidConfiguration = 2, ///< Configuration error or missing value
  kProcessSpawnFailed = 3,   ///< Failed to spawn child process
  kProcessNotFound = 4,      ///< Specified process not found
  kEventHandlerError = 5,    ///< Event handler execution failed
  kSubstitutionError = 6,    ///< Substitution evaluation failed
  kCyclicDependency = 7,     ///< Circular dependency detected
  kTimeout = 8,              ///< Operation timed out
  kShutdownRequested = 9,    ///< System shutdown in progress
  kNotImplemented = 10,      ///< Feature not implemented
  kInternalError = 11,       ///< Internal inconsistency or bug
  kUnknownError = 99         ///< Unclassified error
};

/**
 * @brief Error information container
 *
 * @details Stores error code and human-readable message.
 *          Provides query methods for error checking.
 *
 * @note AUTOSAR C++14: A18-5-2 - Custom error type instead of exceptions
 * @note This class is lightweight and can be passed by value
 *
 * @requirements REQ-LAUNCH-ERROR-002
 */
class Error final
{
 public:
  /**
   * @brief Default constructor (success)
   *
   * @post Error represents success state
   * @note AUTOSAR C++14: A12-1-1 - Use member initialization list
   */
  Error() : code_(ErrorCode::kSuccess), message_() {}
  
  /**
   * @brief Construct with error code
   *
   * @param code Error code
   * @post Error created with code and default message
   */
  explicit Error(ErrorCode code)
    : code_(code), message_(GetDefaultMessage(code)) {}
  
  /**
   * @brief Construct with error code and custom message
   *
   * @param code Error code
   * @param message Custom error message
   * @post Error created with code and message
   */
  Error(ErrorCode code, const std::string& message)
    : code_(code), message_(message) {}
  
  /**
   * @brief Get the error code
   *
   * @return ErrorCode value
   * @note AUTOSAR C++14: M0-1-9 - Declare noexcept if non-throwing
   */
  ErrorCode GetCode() const noexcept { return code_; }
  
  /**
   * @brief Get the error message
   *
   * @return Reference to error message string
   * @note Returns reference to internal storage
   * @note AUTOSAR C++14: M0-1-9 - Declare noexcept if non-throwing
   */
  const std::string& GetMessage() const noexcept { return message_; }
  
  /**
   * @brief Check if this represents success
   *
   * @return true if code is kSuccess
   * @note Convenience method for success checking
   */
  bool IsSuccess() const noexcept { return code_ == ErrorCode::kSuccess; }
  
  /**
   * @brief Check if this represents an error
   *
   * @return true if code is not kSuccess
   * @note Convenience method for error checking
   */
  bool IsError() const noexcept { return code_ != ErrorCode::kSuccess; }
  
  /**
   * @brief Boolean conversion operator
   *
   * @return true if success
   * @note Allows use in if statements: if (error) { ... }
   */
  explicit operator bool() const noexcept { return IsSuccess(); }
  
 private:
  /**
   * @brief Get default message for error code
   *
   * @param code Error code
   * @return Static string with description
   * @note Returns pointer to static string - do not free
   */
  static const char* GetDefaultMessage(ErrorCode code);
  
  ErrorCode code_;       ///< Error code value
  std::string message_;  ///< Human-readable message
};

/**
 * @brief Get default error message for code
 *
 * @param code Error code
 * @return Static string description
 * @note AUTOSAR C++14: M7-1-2 - Use const reference for non-modifiable
 * @note Returns English descriptions
 */
inline const char* Error::GetDefaultMessage(ErrorCode code)
{
  switch (code)
  {
    case ErrorCode::kSuccess:
      return "Success";
    case ErrorCode::kInvalidArgument:
      return "Invalid argument";
    case ErrorCode::kInvalidConfiguration:
      return "Invalid configuration";
    case ErrorCode::kProcessSpawnFailed:
      return "Failed to spawn process";
    case ErrorCode::kProcessNotFound:
      return "Process not found";
    case ErrorCode::kEventHandlerError:
      return "Event handler error";
    case ErrorCode::kSubstitutionError:
      return "Substitution evaluation error";
    case ErrorCode::kCyclicDependency:
      return "Cyclic dependency detected";
    case ErrorCode::kTimeout:
      return "Operation timed out";
    case ErrorCode::kShutdownRequested:
      return "Shutdown requested";
    case ErrorCode::kNotImplemented:
      return "Feature not implemented";
    case ErrorCode::kInternalError:
      return "Internal error";
    case ErrorCode::kUnknownError:
    default:
      return "Unknown error";
  }
}

/**
 * @brief Result type for operations that may fail
 *
 * @tparam T Type of successful result value
 *
 * @details Implements the Result monad pattern for error handling.
 *          Contains either a value of type T or an Error.
 *          Similar to std::expected (C++23) or Result types in Rust.
 *
 * @note AUTOSAR C++14: Exception-free error propagation
 * @note Check HasValue() before calling GetValue()
 * @note Default constructs to success state
 *
 * @requirements REQ-LAUNCH-ERROR-003
 */
template<typename T>
class Result final
{
 public:
  /**
   * @brief Default constructor (success with default value)
   * @post Result contains default-constructed T
   * @note AUTOSAR C++14: A12-1-1 - Member initialization
   */
  Result() : value_(), error_(ErrorCode::kSuccess) {}
  
  /**
   * @brief Construct from value (copy)
   *
   * @param value Value to store
   * @post Result contains value, success state
   */
  explicit Result(const T& value)
    : value_(value), error_(ErrorCode::kSuccess) {}
  
  /**
   * @brief Construct from value (move)
   *
   * @param value Value to move
   * @post Result contains moved value, success state
   */
  explicit Result(T&& value)
    : value_(std::move(value)), error_(ErrorCode::kSuccess) {}
  
  /**
   * @brief Construct from error (copy)
   *
   * @param error Error to store
   * @post Result contains error, no value
   */
  explicit Result(const Error& error)
    : value_(), error_(error) {}
  
  /**
   * @brief Construct from error (move)
   *
   * @param error Error to move
   * @post Result contains moved error, no value
   */
  explicit Result(Error&& error)
    : value_(), error_(std::move(error)) {}
  
  /**
   * @brief Check if result contains a value
   *
   * @return true if success
   */
  bool HasValue() const noexcept { return error_.IsSuccess(); }
  
  /**
   * @brief Check if result contains an error
   *
   * @return true if error
   */
  bool HasError() const noexcept { return error_.IsError(); }
  
  /**
   * @brief Get the value (const)
   *
   * @return Const reference to value
   * @pre HasValue() must be true
   * @note AUTOSAR C++14: A8-4-13 - Do not return non-const references carelessly
   */
  const T& GetValue() const { return value_; }
  
  /**
   * @brief Get the value (non-const)
   *
   * @return Reference to value
   * @pre HasValue() must be true
   */
  T& GetValue()
  {
    // AUTOSAR C++14: M5-0-3 - Use explicit type conversions
    return const_cast<T&>(static_cast<const Result*>(this)->GetValue());
  }
  
  /**
   * @brief Get the error
   *
   * @return Const reference to error
   */
  const Error& GetError() const noexcept { return error_; }
  
  /**
   * @brief Boolean conversion operator
   *
   * @return true if has value
   */
  explicit operator bool() const noexcept { return HasValue(); }
  
 private:
  T value_;      ///< Stored value (valid if success)
  Error error_;  ///< Error information
};

/**
 * @brief Result specialization for void
 *
 * @details Specialization for operations that succeed or fail
 *          without returning a value.
 */
template<>
class Result<void> final
{
 public:
  /**
   * @brief Default constructor (success)
   */
  Result() : error_(ErrorCode::kSuccess) {}
  
  /**
   * @brief Construct from error (copy)
   *
   * @param error Error to store
   */
  explicit Result(const Error& error)
    : error_(error) {}
  
  /**
   * @brief Construct from error (move)
   *
   * @param error Error to move
   */
  explicit Result(Error&& error)
    : error_(std::move(error)) {}
  
  /**
   * @brief Check if result represents success
   *
   * @return true if success
   */
  bool HasValue() const noexcept { return error_.IsSuccess(); }
  
  /**
   * @brief Check if result contains an error
   *
   * @return true if error
   */
  bool HasError() const noexcept { return error_.IsError(); }
  
  /**
   * @brief Get the error
   *
   * @return Const reference to error
   */
  const Error& GetError() const noexcept { return error_; }
  
  /**
   * @brief Boolean conversion operator
   *
   * @return true if success
   */
  explicit operator bool() const noexcept { return HasValue(); }
  
 private:
  Error error_;  ///< Error information
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__ERROR_CODE_HPP_
