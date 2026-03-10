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


#include "launch_cpp/actions/declare_launch_argument.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/substitution.hpp"

namespace launch_cpp
{

DeclareLaunchArgument::DeclareLaunchArgument(const Options& options)
  : Action(), options_(options)
{
}

Result<void> DeclareLaunchArgument::execute(LaunchContext& context)
{
  // Check if already set
  if (context.has_launch_configuration(options_.name))
  {
    return Result<void>();
  }
  
  // Use default value if provided
  if (options_.defaultValue)
  {
    std::string value = options_.defaultValue->perform(context);
    context.set_launch_configuration(options_.name, value);
  }
  
  return Result<void>();
}

}  // namespace launch_cpp
