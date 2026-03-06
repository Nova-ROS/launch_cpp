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

#ifndef CPP_LAUNCH__EVENT_HPP_
#define CPP_LAUNCH__EVENT_HPP_

// AUTOSAR C++14 Compliant Event Base Class
// No exceptions, virtual destructor

#include <chrono>
#include <string>
#include <cstdint>
#include "cpp_launch/types.hpp"

namespace cpp_launch
{

// AUTOSAR C++14: A12-8-4 - Base class with virtual destructor
class Event
{
 public:
  // AUTOSAR C++14: A12-1-1 - Use member initialization list
  Event() : timestamp_(std::chrono::steady_clock::now()) {}
  
  // AUTOSAR C++14: A12-8-4 - Virtual destructor
  virtual ~Event() {}
  
  // AUTOSAR C++14: A10-3-3 - Declare special functions
  Event(const Event&) = default;
  Event& operator=(const Event&) = default;
  Event(Event&&) = default;
  Event& operator=(Event&&) = default;
  
  // AUTOSAR C++14: M0-1-9 - Pure virtual function
  virtual const char* GetType() const = 0;
  
  // AUTOSAR C++14: M0-1-9 - Declare functions as noexcept
  std::chrono::steady_clock::time_point GetTimestamp() const noexcept
  {
    return timestamp_;
  }
  
 protected:
  std::chrono::steady_clock::time_point timestamp_;
};

// Built-in event types
class ProcessStartedEvent final : public Event
{
 public:
  ProcessStartedEvent(std::int32_t pid, const std::string& name)
    : pid_(pid), name_(name) {}
  
  const char* GetType() const override { return "process_started"; }
  
  std::int32_t GetPid() const noexcept { return pid_; }
  const std::string& GetName() const noexcept { return name_; }
  
 private:
  std::int32_t pid_;
  std::string name_;
};

class ProcessExitedEvent final : public Event
{
 public:
  ProcessExitedEvent(std::int32_t pid, std::int32_t returnCode, const std::string& name)
    : pid_(pid), returnCode_(returnCode), name_(name) {}
  
  const char* GetType() const override { return "process_exited"; }
  
  std::int32_t GetPid() const noexcept { return pid_; }
  std::int32_t GetReturnCode() const noexcept { return returnCode_; }
  const std::string& GetName() const noexcept { return name_; }
  
 private:
  std::int32_t pid_;
  std::int32_t returnCode_;
  std::string name_;
};

class ShutdownEvent final : public Event
{
 public:
  explicit ShutdownEvent(const std::string& reason)
    : reason_(reason) {}
  
  const char* GetType() const override { return "shutdown"; }
  
  const std::string& GetReason() const noexcept { return reason_; }
  
 private:
  std::string reason_;
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__EVENT_HPP_
