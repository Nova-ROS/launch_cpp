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


#ifndef LAUNCH_CPP__EVENT_HANDLER_HPP_
#define LAUNCH_CPP__EVENT_HANDLER_HPP_

// AUTOSAR C++14 Compliant Event Handler Interface

#include <vector>
#include <functional>
#include "launch_cpp/types.hpp"
#include "launch_cpp/event.hpp"
#include "launch_cpp/error_code.hpp"

namespace launch_cpp
{

// Forward declaration
class LaunchContext;

// AUTOSAR C++14: A12-8-4 - Base class with virtual destructor
class EventHandler
{
 public:
  EventHandler() = default;
  
  // AUTOSAR C++14: A12-8-4 - Virtual destructor
  virtual ~EventHandler() {}
  
  // AUTOSAR C++14: A10-3-3 - Declare special functions
  EventHandler(const EventHandler&) = default;
  EventHandler& operator=(const EventHandler&) = default;
  EventHandler(EventHandler&&) = default;
  EventHandler& operator=(EventHandler&&) = default;
  
  // AUTOSAR C++14: M0-1-9 - Pure virtual function
  virtual bool Matches(const Event& event) const = 0;
  
  virtual Result<LaunchDescriptionEntityVector> Handle(
    const Event& event, 
    LaunchContext& context) = 0;
};

// Convenience handler using std::function
class FunctionEventHandler final : public EventHandler
{
 public:
  using MatcherFunc = std::function<bool(const Event&)>;
  using HandlerFunc = std::function<Result<LaunchDescriptionEntityVector>(
    const Event&, LaunchContext&)>;
  
  FunctionEventHandler(MatcherFunc matcher, HandlerFunc handler)
    : matcher_(std::move(matcher)), handler_(std::move(handler)) {}
  
  bool Matches(const Event& event) const override
  {
    return matcher_(event);
  }
  
  Result<LaunchDescriptionEntityVector> Handle(
    const Event& event, 
    LaunchContext& context) override
  {
    return handler_(event, context);
  }
  
 private:
  MatcherFunc matcher_;
  HandlerFunc handler_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__EVENT_HANDLER_HPP_
