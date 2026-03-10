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


#include "launch_cpp/conditions/unless_condition.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/substitution.hpp"

namespace launch_cpp
{

bool UnlessCondition::evaluate(const LaunchContext& context) const
{
  if (!expression_)
  {
    return true;  // If no expression, condition is true (execute unless nothing)
  }

  std::string result = expression_->perform(context);

  // Truthy check: empty or "false" or "0" means false, so Unless returns true
  if (result.empty())
  {
    return true;
  }

  if (result == "false" || result == "0")
  {
    return true;
  }

  return false;  // Expression is true, so Unless is false
}

}  // namespace launch_cpp
