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

#ifndef CPP_LAUNCH__EVENT_DISPATCHER_HPP_
#define CPP_LAUNCH__EVENT_DISPATCHER_HPP_

// AUTOSAR C++14 Compliant Event Dispatcher

#include <vector>
#include <memory>
#include "cpp_launch/event.hpp"
#include "cpp_launch/event_handler.hpp"
#include "cpp_launch/error_code.hpp"

namespace cpp_launch
{

class LaunchContext;

class EventDispatcher
{
 public:
  EventDispatcher() = default;

  // AUTOSAR C++14: A12-8-4 - Virtual destructor
  virtual ~EventDispatcher() {}

  // AUTOSAR C++14: A10-3-3 - Declare special functions
  EventDispatcher(const EventDispatcher&) = delete;
  EventDispatcher& operator=(const EventDispatcher&) = delete;
  EventDispatcher(EventDispatcher&&) = delete;
  EventDispatcher& operator=(EventDispatcher&&) = delete;

  // Register an event handler
  void RegisterHandler(const EventHandlerPtr& handler)
  {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    handlers_.push_back(handler);
  }

  // Unregister an event handler
  void UnregisterHandler(const EventHandler* handler)
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

  // Dispatch event to matching handlers
  Result<LaunchDescriptionEntityVector> Dispatch(const Event& event, LaunchContext& context)
  {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    LaunchDescriptionEntityVector all_entities;

    for (const auto& handler : handlers_)
    {
      if (handler && handler->Matches(event))
      {
        Result<LaunchDescriptionEntityVector> result = handler->Handle(event, context);
        if (result.HasError())
        {
          return result;
        }

        // Append entities
        const LaunchDescriptionEntityVector& entities = result.GetValue();
        all_entities.insert(all_entities.end(), entities.begin(), entities.end());
      }
    }

    return Result<LaunchDescriptionEntityVector>(std::move(all_entities));
  }

  // Get number of registered handlers
  std::size_t GetHandlerCount() const
  {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    return handlers_.size();
  }

  // Clear all handlers
  void ClearHandlers()
  {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    handlers_.clear();
  }

 private:
  mutable std::mutex handlersMutex_;
  std::vector<EventHandlerPtr> handlers_;
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__EVENT_DISPATCHER_HPP_
