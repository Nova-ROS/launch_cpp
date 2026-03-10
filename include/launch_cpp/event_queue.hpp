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


#ifndef LAUNCH_CPP__EVENT_QUEUE_HPP_
#define LAUNCH_CPP__EVENT_QUEUE_HPP_

// AUTOSAR C++14 Compliant Thread-Safe Event Queue

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "launch_cpp/event.hpp"

namespace launch_cpp
{

class EventQueue
{
 public:
  EventQueue() = default;

  // AUTOSAR C++14: A12-8-4 - Virtual destructor
  virtual ~EventQueue() {}

  // AUTOSAR C++14: A10-3-3 - Declare special functions
  EventQueue(const EventQueue&) = delete;
  EventQueue& operator=(const EventQueue&) = delete;
  EventQueue(EventQueue&&) = delete;
  EventQueue& operator=(EventQueue&&) = delete;

  // Push event to queue
  void push(const EventPtr& event)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(event);
    condition_.notify_one();
  }

  // Try to pop event (non-blocking)
  bool try_pop(EventPtr& event)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty())
    {
      return false;
    }
    event = queue_.front();
    queue_.pop();
    return true;
  }

  // Wait and pop event (blocking with timeout)
  bool wait_and_pop(EventPtr& event, std::chrono::milliseconds timeout)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    bool has_event = condition_.wait_for(lock, timeout, [this]()
    {
      return !queue_.empty();
    });

    if (has_event)
    {
      event = queue_.front();
      queue_.pop();
      return true;
    }
    return false;
  }

  // Check if queue is empty
  bool is_empty() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  // Get queue size
  std::size_t size() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

  // Clear all events
  void clear()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty())
    {
      queue_.pop();
    }
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::queue<EventPtr> queue_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__EVENT_QUEUE_HPP_
