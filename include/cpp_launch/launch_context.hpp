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

#ifndef CPP_LAUNCH__LAUNCH_CONTEXT_HPP_
#define CPP_LAUNCH__LAUNCH_CONTEXT_HPP_

// AUTOSAR C++14 Compliant Launch Context Interface

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "cpp_launch/types.hpp"
#include "cpp_launch/error_code.hpp"
#include "cpp_launch/event.hpp"
#include "cpp_launch/event_handler.hpp"

namespace cpp_launch
{

// AUTOSAR C++14: A12-8-4 - Base class with virtual destructor
class LaunchContext
{
 public:
  struct Arguments
  {
    std::vector<std::string> argv;
    bool noninteractive = false;
  };
  
  LaunchContext() = default;
  
  // AUTOSAR C++14: A12-8-4 - Virtual destructor
  virtual ~LaunchContext() {}
  
  // AUTOSAR C++14: A10-3-3 - Declare special functions
  LaunchContext(const LaunchContext&) = delete;
  LaunchContext& operator=(const LaunchContext&) = delete;
  LaunchContext(LaunchContext&&) = delete;
  LaunchContext& operator=(LaunchContext&&) = delete;
  
  // Event handlers
  virtual void RegisterEventHandler(const EventHandlerPtr& handler) = 0;
  virtual void UnregisterEventHandler(const EventHandler* handler) = 0;
  virtual const EventHandlerVector& GetEventHandlers() const = 0;
  
  // Launch configurations
  virtual void SetLaunchConfiguration(const std::string& key, const std::string& value) = 0;
  virtual Result<std::string> GetLaunchConfiguration(const std::string& key) const = 0;
  virtual bool HasLaunchConfiguration(const std::string& key) const = 0;
  
  // Current launch file tracking
  virtual void SetCurrentLaunchFile(const std::string& path) = 0;
  virtual std::string GetCurrentLaunchFile() const = 0;
  
  // Environment variables
  virtual std::string GetEnvironmentVariable(const std::string& name) const = 0;
  virtual void SetEnvironmentVariable(const std::string& name, const std::string& value) = 0;
  
  // Event emission
  virtual void EmitEvent(EventPtr event) = 0;
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__LAUNCH_CONTEXT_HPP_
