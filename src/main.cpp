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


#include <iostream>
#include <cstdlib>

// Include core headers
#include "launch_cpp/launch_service.hpp"
#include "launch_cpp/launch_description.hpp"
#include "launch_cpp/actions/execute_process.hpp"
#include "launch_cpp/actions/declare_launch_argument.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"
#include "launch_cpp/substitutions/launch_configuration.hpp"
#include "launch_cpp/conditions/if_condition.hpp"

using namespace launch_cpp;

void print_usage(const char* program)
{
  std::cout << "Usage: " << program << " <launch_file.yaml> [args...]" << std::endl;
  std::cout << "       " << program << " --help" << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --help              Show this help message" << std::endl;
  std::cout << "  --debug             Enable debug mode" << std::endl;
  std::cout << std::endl;
  std::cout << "Examples:" << std::endl;
  std::cout << "  " << program << " my_launch.yaml" << std::endl;
  std::cout << "  " << program << " my_launch.yaml use_sim_time:=true" << std::endl;
}

std::int32_t main(std::int32_t argc, char* argv[])
{
  if (argc < 2)
  {
    print_usage(argv[0]);
    return 1;
  }

  std::string launchFile = argv[1];

  if (launchFile == "--help" || launchFile == "-h")
  {
    print_usage(argv[0]);
    return 0;
  }

  // Parse arguments
  LaunchService::Options options;
  options.debug = false;

  for (std::int32_t i = 2; i < argc; ++i)
  {
    std::string arg = argv[i];

    if (arg == "--debug")
    {
      options.debug = true;
    } else {
      options.argv.push_back(arg);
    }
  }

  // Create service
  LaunchService service(options);

  // Load launch file
  Result<LaunchDescriptionPtr> loadResult = LaunchDescription::from_yaml_file(launchFile);

  if (loadResult.has_error())
  {
    std::cerr << "Error loading launch file: " << loadResult.get_error().get_message() << std::endl;
    return 1;
  }

  // Include launch description
  Error includeError = service.include_launch_description(loadResult.get_value());

  if (includeError.is_error())
  {
    std::cerr << "Error including launch description: " << includeError.get_message() << std::endl;
    return 1;
  }

  // Run
  std::int32_t exitCode = service.run();

  return exitCode;
}
