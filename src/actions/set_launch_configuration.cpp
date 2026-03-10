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


#include "launch_cpp/actions/set_launch_configuration.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/substitution.hpp"

namespace launch_cpp
{

SetLaunchConfiguration::SetLaunchConfiguration(const Options& options)
  : Action(), options_(options)
{
}

Result<void> SetLaunchConfiguration::execute(LaunchContext& context)
{
  if (options_.value)
  {
    std::string value = options_.value->perform(context);
    context.set_launch_configuration(options_.name, value);
  } else {
    context.set_launch_configuration(options_.name, "");
  }

  return Result<void>();
}

}  // namespace launch_cpp
