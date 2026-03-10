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


// Basic example: Execute a simple process
#include <iostream>
#include "launch_cpp/launch_service.hpp"
#include "launch_cpp/launch_description.hpp"
#include "launch_cpp/actions/execute_process.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"

using namespace launch_cpp;

int main()
{
  std::cout << "=== Basic Example ===" << std::endl;
  
  // Create launch service
  LaunchService service;
  
  // Create launch description
  LaunchDescriptionPtr desc = std::make_shared<LaunchDescription>();
  
  // Add execute process action
  ExecuteProcess::Options options;
  options.cmd.push_back(std::make_shared<TextSubstitution>("echo"));
  options.cmd.push_back(std::make_shared<TextSubstitution>("Hello, C++ Launch!"));
  options.output = "screen";
  
  desc->add(std::make_shared<ExecuteProcess>(options));
  
  // Include and run
  Error includeError = service.include_launch_description(desc);
  if (includeError.is_error())
  {
    std::cerr << "Error: " << includeError.get_message() << std::endl;
    return 1;
  }
  
  std::int32_t exitCode = service.run();
  
  std::cout << "Exit code: " << exitCode << std::endl;
  return exitCode;
}
