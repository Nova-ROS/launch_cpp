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


#include "launch_cpp/conditions/launch_configuration_equals.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/substitution.hpp"

namespace launch_cpp
{

bool LaunchConfigurationEquals::evaluate(const LaunchContext& context) const
{
  Result<std::string> actual = context.get_launch_configuration(name_);

  if (actual.has_error())
  {
    return false;
  }

  if (!expected_)
  {
    return false;
  }

  std::string expectedValue = expected_->perform(context);

  return actual.get_value() == expectedValue;
}

}  // namespace launch_cpp
