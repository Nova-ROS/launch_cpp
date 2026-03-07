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


#include "cpp_launch/actions/set_launch_configuration.hpp"
#include "cpp_launch/launch_context.hpp"
#include "cpp_launch/substitution.hpp"

namespace cpp_launch
{

SetLaunchConfiguration::SetLaunchConfiguration(const Options& options)
  : Action(), options_(options)
{
}

Result<void> SetLaunchConfiguration::Execute(LaunchContext& context)
{
  if (options_.value)
  {
    std::string value = options_.value->Perform(context);
    context.SetLaunchConfiguration(options_.name, value);
  }
  else
  {
    context.SetLaunchConfiguration(options_.name, "");
  }

  return Result<void>();
}

}  // namespace cpp_launch
