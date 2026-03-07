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


#ifndef CPP_LAUNCH__LAUNCH_SERVICE_HPP_
#define CPP_LAUNCH__LAUNCH_SERVICE_HPP_

// AUTOSAR C++14 Compliant Launch Service

#include "cpp_launch/types.hpp"
#include "cpp_launch/error_code.hpp"
#include "cpp_launch/launch_description.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace cpp_launch
{

// Forward declarations
class LaunchContext;
class Event;
class EventHandler;

// Service status
enum class LaunchServiceStatus : std::int32_t
{
  kIdle = 0,
  kRunning = 1,
  kShuttingDown = 2,
  kStopped = 3
};

class LaunchService final
{
 public:
  struct Options
  {
    std::vector<std::string> argv;
    bool noninteractive;
    bool debug;
    
    Options() : noninteractive(false), debug(false) {}
  };
  
  explicit LaunchService(const Options& options = Options());
  
  ~LaunchService() noexcept;
  
  LaunchService(const LaunchService&) = delete;
  LaunchService& operator=(const LaunchService&) = delete;
  LaunchService(LaunchService&&) = delete;
  LaunchService& operator=(LaunchService&&) = delete;
  
  // Main entry point
  std::int32_t Run(bool shutdownWhenIdle = true);
  
  // Add launch description
  Error IncludeLaunchDescription(const LaunchDescriptionPtr& description);
  
  // Event operations
  void EmitEvent(EventPtr event);
  
  Error Shutdown();
  
  // Status queries
  bool IsRunning() const noexcept;
  bool IsIdle() const;
  
  LaunchServiceStatus GetStatus() const noexcept
  {
    return status_.load(std::memory_order_acquire);
  }
  
  LaunchContext& GetContext();
  const LaunchContext& GetContext() const;
  
 private:
  void RunLoop();
  void ProcessOneEvent();
  void SetupSignalHandlers();
  void HandleShutdown(const std::string& reason);
  
  class Impl;
  std::unique_ptr<Impl> impl_;
  
  std::atomic<LaunchServiceStatus> status_;
  std::atomic<bool> shutdownRequested_;
  
  std::vector<LaunchDescriptionPtr> descriptions_;
  std::mutex descriptionsMutex_;
  
  std::thread workerThread_;
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__LAUNCH_SERVICE_HPP_
