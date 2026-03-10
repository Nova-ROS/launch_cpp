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
    return Result<void>(Error(ErrorCode::K_INVALID_ARGUMENT, "No launch description source provided"));
  }

  // Resolve the file path
  std::string file_path = options_.launchDescriptionSource->perform(context);

  // Check if file exists
  std::ifstream file(file_path);
  if (!file.is_open())
  {
    return Result<void>(Error(ErrorCode::K_INVALID_ARGUMENT, "Launch file not found: " + file_path));
  }
  file.close();

  // Parse the launch file
  auto desc_result = LaunchDescription::from_yaml_file(file_path);
  if (desc_result.has_error())
  {
    return Result<void>(desc_result.get_error());
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
  auto visit_result = desc_result.get_value()->visit(context);
  if (visit_result.has_error())
  {
    return Result<void>(visit_result.get_error());
  }

  return Result<void>();
}

}  // namespace launch_cpp
