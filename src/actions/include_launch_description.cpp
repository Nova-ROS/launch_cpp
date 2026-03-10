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


#include "launch_cpp/actions/include_launch_description.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/launch_description.hpp"
#include "launch_cpp/substitution.hpp"
#include <fstream>

namespace launch_cpp
{

IncludeLaunchDescription::IncludeLaunchDescription(const Options& options)
  : Action(), options_(options)
{
}

Result<void> IncludeLaunchDescription::execute(LaunchContext& context)
{
  if (!options_.launchDescriptionSource)
  {
    return Result<void>(Error(ErrorCode::kInvalidArgument, "No launch description source provided"));
  }
  
  // Resolve the file path
  std::string filePath = options_.launchDescriptionSource->perform(context);
  
  // Check if file exists
  std::ifstream file(filePath);
  if (!file.is_open())
  {
    return Result<void>(Error(ErrorCode::kInvalidArgument, "Launch file not found: " + filePath));
  }
  file.close();
  
  // Parse the launch file
  auto descResult = LaunchDescription::from_yaml_file(filePath);
  if (descResult.has_error())
  {
    return Result<void>(descResult.get_error());
  }
  
  // Set launch arguments
  for (const auto& arg : options_.launchArguments)
  {
    if (arg.second)
    {
      std::string value = arg.second->perform(context);
      context.set_launch_configuration(arg.first, value);
    }
  }
  
  // Visit the included launch description
  auto visitResult = descResult.get_value()->visit(context);
  if (visitResult.has_error())
  {
    return Result<void>(visitResult.get_error());
  }
  
  return Result<void>();
}

}  // namespace launch_cpp
