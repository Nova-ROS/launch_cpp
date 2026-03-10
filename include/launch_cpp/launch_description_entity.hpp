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


#ifndef LAUNCH_CPP__LAUNCH_DESCRIPTION_ENTITY_HPP_
#define LAUNCH_CPP__LAUNCH_DESCRIPTION_ENTITY_HPP_

// AUTOSAR C++14 Compliant Launch Description Entity Base

#include "launch_cpp/types.hpp"
#include "launch_cpp/error_code.hpp"

namespace launch_cpp
{

// Forward declaration
class LaunchContext;

// AUTOSAR C++14: A12-8-4 - Base class with virtual destructor
class LaunchDescriptionEntity
{
 public:
  LaunchDescriptionEntity() = default;
  
  // AUTOSAR C++14: A12-8-4 - Virtual destructor
  virtual ~LaunchDescriptionEntity() {}
  
  // AUTOSAR C++14: A10-3-3 - Declare special functions
  LaunchDescriptionEntity(const LaunchDescriptionEntity&) = default;
  LaunchDescriptionEntity& operator=(const LaunchDescriptionEntity&) = default;
  LaunchDescriptionEntity(LaunchDescriptionEntity&&) = default;
  LaunchDescriptionEntity& operator=(LaunchDescriptionEntity&&) = default;
  
  // AUTOSAR C++14: M0-1-9 - Pure virtual function
  virtual Result<LaunchDescriptionEntityVector> visit(LaunchContext& context) = 0;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__LAUNCH_DESCRIPTION_ENTITY_HPP_
