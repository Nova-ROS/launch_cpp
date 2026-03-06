// Copyright 2024 Example Author
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

#ifndef CPP_LAUNCH__ERROR_CODE_HPP_
#define CPP_LAUNCH__ERROR_CODE_HPP_

// AUTOSAR C++14 Compliant Error Handling
// Uses error codes instead of exceptions (AUTOSAR A15-0-1)

#include <cstdint>
#include <string>

namespace cpp_launch
{

// AUTOSAR C++14: A7-2-4 - Use enum class for enumeration types
enum class ErrorCode : std::int32_t
{
  kSuccess = 0,
  kInvalidArgument = 1,
  kInvalidConfiguration = 2,
  kProcessSpawnFailed = 3,
  kProcessNotFound = 4,
  kEventHandlerError = 5,
  kSubstitutionError = 6,
  kCyclicDependency = 7,
  kTimeout = 8,
  kShutdownRequested = 9,
  kNotImplemented = 10,
  kInternalError = 11,
  kUnknownError = 99
};

// AUTOSAR C++14: A18-5-2 - Do not use std::exception
// Use custom error handling without exceptions
class Error final
{
 public:
  // AUTOSAR C++14: A12-1-1 - Use member initialization list
  Error() : code_(ErrorCode::kSuccess), message_() {}
  
  explicit Error(ErrorCode code)
    : code_(code), message_(GetDefaultMessage(code)) {}
  
  Error(ErrorCode code, const std::string& message)
    : code_(code), message_(message) {}
  
  // AUTOSAR C++14: M0-1-9 - Declare functions as noexcept if they do not throw
  ErrorCode GetCode() const noexcept { return code_; }
  
  const std::string& GetMessage() const noexcept { return message_; }
  
  bool IsSuccess() const noexcept { return code_ == ErrorCode::kSuccess; }
  
  bool IsError() const noexcept { return code_ != ErrorCode::kSuccess; }
  
  explicit operator bool() const noexcept { return IsSuccess(); }
  
 private:
  static const char* GetDefaultMessage(ErrorCode code);
  
  ErrorCode code_;
  std::string message_;
};

// AUTOSAR C++14: M7-1-2 - Use const reference for non-modifiable parameters
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

// Result type for operations that can fail
template<typename T>
class Result final
{
 public:
  // AUTOSAR C++14: A12-1-1 - Constructor with member initialization
  Result() : value_(), error_(ErrorCode::kSuccess) {}
  
  explicit Result(const T& value)
    : value_(value), error_(ErrorCode::kSuccess) {}
  
  explicit Result(T&& value)
    : value_(std::move(value)), error_(ErrorCode::kSuccess) {}
  
  explicit Result(const Error& error)
    : value_(), error_(error) {}
  
  explicit Result(Error&& error)
    : value_(), error_(std::move(error)) {}
  
  // Accessors
  bool HasValue() const noexcept { return error_.IsSuccess(); }
  bool HasError() const noexcept { return error_.IsError(); }
  
  // AUTOSAR C++14: A8-4-13 - Do not return non-const references
  const T& GetValue() const { return value_; }
  
  T& GetValue()
  {
    // AUTOSAR C++14: M5-0-3 - Use explicit type conversions
    return const_cast<T&>(static_cast<const Result*>(this)->GetValue());
  }
  
  const Error& GetError() const noexcept { return error_; }
  
  // Explicit conversion operators
  explicit operator bool() const noexcept { return HasValue(); }
  
 private:
  T value_;
  Error error_;
};

// Specialization for void
template<>
class Result<void> final
{
 public:
  Result() : error_(ErrorCode::kSuccess) {}
  
  explicit Result(const Error& error)
    : error_(error) {}
  
  explicit Result(Error&& error)
    : error_(std::move(error)) {}
  
  bool HasValue() const noexcept { return error_.IsSuccess(); }
  bool HasError() const noexcept { return error_.IsError(); }
  
  const Error& GetError() const noexcept { return error_; }
  
  explicit operator bool() const noexcept { return HasValue(); }
  
 private:
  Error error_;
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__ERROR_CODE_HPP_
