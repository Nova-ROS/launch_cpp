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


#ifndef LAUNCH_CPP__LAUNCH_CONTEXT_IMPL_HPP_
#define LAUNCH_CPP__LAUNCH_CONTEXT_IMPL_HPP_

#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/event.hpp"
#include "launch_cpp/event_handler.hpp"
#include <unordered_map>
#include <mutex>
#include <cstdlib>

namespace launch_cpp
{

class LaunchContextImpl final : public LaunchContext
{
 public:
  explicit LaunchContextImpl(const Arguments& args = Arguments{})
    : args_(args)
  {
  }
  
  ~LaunchContextImpl() override = default;
  
  void RegisterEventHandler(const EventHandlerPtr& handler) override
  {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    handlers_.push_back(handler);
  }
  
  void UnregisterEventHandler(const EventHandler* handler) override
  {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    
    for (auto it = handlers_.begin(); it != handlers_.end(); ++it)
    {
      if (it->get() == handler)
      {
        handlers_.erase(it);
        return;
      }
    }
  }
  
  const EventHandlerVector& GetEventHandlers() const override
  {
    return handlers_;
  }
  
  void SetLaunchConfiguration(const std::string& key, const std::string& value) override
  {
    std::lock_guard<std::mutex> lock(configMutex_);
    configurations_[key] = value;
  }
  
  Result<std::string> GetLaunchConfiguration(const std::string& key) const override
  {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    auto it = configurations_.find(key);
    if (it == configurations_.end())
    {
      return Result<std::string>(Error(ErrorCode::kInvalidArgument, "Configuration not found"));
    }
    
    return Result<std::string>(it->second);
  }
  
  bool HasLaunchConfiguration(const std::string& key) const override
  {
    std::lock_guard<std::mutex> lock(configMutex_);
    return configurations_.find(key) != configurations_.end();
  }
  
  std::string GetEnvironmentVariable(const std::string& name) const override
  {
    const char* value = std::getenv(name.c_str());
    return value ? std::string(value) : std::string();
  }
  
  void SetEnvironmentVariable(const std::string& name, const std::string& value) override
  {
    setenv(name.c_str(), value.c_str(), 1);
  }
  
  void EmitEvent(EventPtr event) override
  {
    // TODO: Implement event queue
    (void)event;
  }
  
  void SetCurrentLaunchFile(const std::string& path) override
  {
    std::lock_guard<std::mutex> lock(launchFileMutex_);
    currentLaunchFile_ = path;
  }
  
  std::string GetCurrentLaunchFile() const override
  {
    std::lock_guard<std::mutex> lock(launchFileMutex_);
    return currentLaunchFile_;
  }
  
 private:
  Arguments args_;
  EventHandlerVector handlers_;
  mutable std::mutex handlersMutex_;
  
  std::unordered_map<std::string, std::string> configurations_;
  mutable std::mutex configMutex_;
  
  std::string currentLaunchFile_;
  mutable std::mutex launchFileMutex_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__LAUNCH_CONTEXT_IMPL_HPP_
