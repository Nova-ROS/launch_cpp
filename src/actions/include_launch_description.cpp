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

Result<void> IncludeLaunchDescription::Execute(LaunchContext& context)
{
  if (!options_.launchDescriptionSource)
  {
    return Result<void>(Error(ErrorCode::kInvalidArgument, "No launch description source provided"));
  }
  
  // Resolve the file path
  std::string filePath = options_.launchDescriptionSource->Perform(context);
  
  // Check if file exists
  std::ifstream file(filePath);
  if (!file.is_open())
  {
    return Result<void>(Error(ErrorCode::kInvalidArgument, "Launch file not found: " + filePath));
  }
  file.close();
  
  // Parse the launch file
  auto descResult = LaunchDescription::FromYamlFile(filePath);
  if (descResult.HasError())
  {
    return Result<void>(descResult.GetError());
  }
  
  // Set launch arguments
  for (const auto& arg : options_.launchArguments)
  {
    if (arg.second)
    {
      std::string value = arg.second->Perform(context);
      context.SetLaunchConfiguration(arg.first, value);
    }
  }
  
  // Visit the included launch description
  auto visitResult = descResult.GetValue()->Visit(context);
  if (visitResult.HasError())
  {
    return Result<void>(visitResult.GetError());
  }
  
  return Result<void>();
}

}  // namespace launch_cpp
