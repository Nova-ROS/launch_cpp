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


#include "launch_cpp/launch_service.hpp"
#include "launch_context_impl.hpp"
#include "launch_cpp/launch_description.hpp"
#include "launch_cpp/event.hpp"
#include <iostream>

namespace launch_cpp
{

class LaunchService::Impl
{
 public:
  explicit Impl(const Options& options)
    : context_(LaunchContext::Arguments{options.argv, options.noninteractive})
  {
    (void)options;  // For future use
  }
  
  LaunchContextImpl context_;
};

LaunchService::LaunchService(const Options& options)
  : impl_(std::make_unique<Impl>(options)),
    status_(LaunchServiceStatus::kIdle),
    shutdownRequested_(false)
{
}

LaunchService::~LaunchService() noexcept
{
  Shutdown();
}

std::int32_t LaunchService::Run(bool shutdownWhenIdle)
{
  LaunchServiceStatus expected = LaunchServiceStatus::kIdle;
  
  if (!status_.compare_exchange_strong(
        expected, 
        LaunchServiceStatus::kRunning,
        std::memory_order_release,
        std::memory_order_relaxed))
  {
    std::cerr << "Launch service is not in idle state" << std::endl;
    return 1;
  }
  
  // Visit all launch descriptions
  std::vector<LaunchDescriptionPtr> descriptions;
  
  {
    std::lock_guard<std::mutex> lock(descriptionsMutex_);
    descriptions = descriptions_;
  }
  
  for (const auto& desc : descriptions)
  {
    if (!desc)
    {
      continue;
    }
    
    Result<LaunchDescriptionEntityVector> result = desc->Visit(impl_->context_);
    
    if (result.HasError())
    {
      std::cerr << "Error visiting launch description: " 
                << result.GetError().GetMessage() << std::endl;
      return 1;
    }
  }
  
  if (shutdownWhenIdle)
  {
    Shutdown();
  }
  
  return 0;
}

Error LaunchService::IncludeLaunchDescription(const LaunchDescriptionPtr& description)
{
  if (!description)
  {
    return Error(ErrorCode::kInvalidArgument, "Null launch description");
  }
  
  std::lock_guard<std::mutex> lock(descriptionsMutex_);
  descriptions_.push_back(description);
  
  return Error();
}

void LaunchService::EmitEvent(EventPtr event)
{
  (void)event;
  // TODO: Implement event handling
}

Error LaunchService::Shutdown()
{
  LaunchServiceStatus expected = LaunchServiceStatus::kRunning;
  
  if (!status_.compare_exchange_strong(
        expected,
        LaunchServiceStatus::kShuttingDown,
        std::memory_order_release,
        std::memory_order_relaxed))
  {
    // Already shutting down or stopped
    return Error();
  }
  
  shutdownRequested_.store(true, std::memory_order_release);
  
  status_.store(LaunchServiceStatus::kStopped, std::memory_order_release);
  
  return Error();
}

bool LaunchService::IsRunning() const noexcept
{
  return status_.load(std::memory_order_acquire) == LaunchServiceStatus::kRunning;
}

bool LaunchService::IsIdle() const
{
  return status_.load(std::memory_order_acquire) == LaunchServiceStatus::kIdle;
}

LaunchContext& LaunchService::GetContext()
{
  return impl_->context_;
}

const LaunchContext& LaunchService::GetContext() const
{
  return impl_->context_;
}

}  // namespace launch_cpp
