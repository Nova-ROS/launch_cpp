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


#include <gtest/gtest.h>
#include <memory>
#include <string>

// Include implementation files for testing
#include "../src/launch_context_impl.hpp"
#include "launch_cpp/launch_service.hpp"
#include "launch_cpp/launch_description.hpp"
#include "launch_cpp/actions/execute_process.hpp"
#include "launch_cpp/actions/declare_launch_argument.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"
#include "launch_cpp/substitutions/launch_configuration.hpp"
#include "launch_cpp/conditions/if_condition.hpp"

using namespace launch_cpp;

TEST(LaunchContextTest, ConfigurationManagement)
{
  LaunchContextImpl context;
  
  // Test setting configuration
  context.set_launch_configuration("test_key", "test_value");
  
  // Test getting configuration
  Result<std::string> result = context.get_launch_configuration("test_key");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.get_value(), "test_value");
  
  // Test non-existent key
  Result<std::string> missing = context.get_launch_configuration("missing_key");
  EXPECT_TRUE(missing.has_error());
  
  // Test HasLaunchConfiguration
  EXPECT_TRUE(context.has_launch_configuration("test_key"));
  EXPECT_FALSE(context.has_launch_configuration("missing_key"));
}

TEST(LaunchContextTest, EnvironmentVariables)
{
  LaunchContextImpl context;
  
  // Set an environment variable
  context.set_environment_variable("CPP_LAUNCH_TEST_VAR", "test_value");
  
  // Get it back
  std::string value = context.get_environment_variable("CPP_LAUNCH_TEST_VAR");
  EXPECT_EQ(value, "test_value");
  
  // Get non-existent
  std::string missing = context.get_environment_variable("NON_EXISTENT_VAR_12345");
  EXPECT_TRUE(missing.empty());
}

TEST(LaunchServiceTest, BasicLifecycle)
{
  LaunchService service;
  
  EXPECT_TRUE(service.IsIdle());
  EXPECT_FALSE(service.is_running());
}

TEST(LaunchServiceTest, IncludeLaunchDescription)
{
  LaunchService service;
  
  auto desc = std::make_shared<LaunchDescription>();
  Error error = service.IncludeLaunchDescription(desc);
  
  EXPECT_TRUE(error.is_success());
}

TEST(LaunchServiceTest, IncludeNullDescription)
{
  LaunchService service;
  
  LaunchDescriptionPtr nullDesc;
  Error error = service.IncludeLaunchDescription(nullDesc);
  
  EXPECT_TRUE(error.is_error());
}

TEST(LaunchDescriptionTest, AddAndVisit)
{
  LaunchDescription desc;
  
  // Add an action
  ExecuteProcess::Options options;
  options.cmd.push_back(std::make_shared<TextSubstitution>("echo"));
  options.cmd.push_back(std::make_shared<TextSubstitution>("test"));
  
  auto action = std::make_shared<ExecuteProcess>(options);
  desc.Add(action);
  
  EXPECT_EQ(desc.GetEntities().size(), 1U);
}

TEST(ExecuteProcessTest, BasicConstruction)
{
  ExecuteProcess::Options options;
  options.cmd.push_back(std::make_shared<TextSubstitution>("echo"));
  options.cmd.push_back(std::make_shared<TextSubstitution>("hello"));
  
  ExecuteProcess process(options);
  
  EXPECT_FALSE(process.is_running());
}
