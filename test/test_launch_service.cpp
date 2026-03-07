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


// Comprehensive Launch Service Tests
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "launch_cpp/launch_service.hpp"
#include "launch_cpp/launch_description.hpp"
#include "launch_cpp/actions/set_launch_configuration.hpp"
#include "launch_cpp/actions/declare_launch_argument.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"

using namespace launch_cpp;

// Test: LaunchService basic creation
TEST(LaunchServiceTest, BasicCreation)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  EXPECT_FALSE(service.IsRunning());
}

// Test: LaunchService with options
TEST(LaunchServiceTest, CreationWithOptions)
{
  LaunchService::Options options;
  options.argv = {"test", "arg1", "arg2"};
  options.noninteractive = true;
  
  LaunchService service(options);
  // Just verify it doesn't crash
  EXPECT_FALSE(service.IsRunning());
}

// Test: IncludeLaunchDescription basic
TEST(LaunchServiceTest, IncludeLaunchDescription)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  auto desc = std::make_shared<LaunchDescription>();
  
  Error err = service.IncludeLaunchDescription(desc);
  EXPECT_TRUE(err.IsSuccess());
}

// Test: IncludeLaunchDescription with actions
TEST(LaunchServiceTest, IncludeLaunchDescriptionWithActions)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  auto desc = std::make_shared<LaunchDescription>();
  
  // Add an action
  SetLaunchConfiguration::Options action_opts;
  action_opts.name = "test_config";
  action_opts.value = std::make_shared<TextSubstitution>("test_value");
  desc->Add(std::make_shared<SetLaunchConfiguration>(action_opts));
  
  Error err = service.IncludeLaunchDescription(desc);
  EXPECT_TRUE(err.IsSuccess());
}

// Test: LaunchService run basic
TEST(LaunchServiceTest, RunBasic)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  // Run with shutdown_when_idle = true (default)
  int result = service.Run(true);
  
  EXPECT_EQ(result, 0);
}

// Test: LaunchService run without auto-shutdown
TEST(LaunchServiceTest, RunWithoutShutdown)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  // Run without auto-shutdown
  int result = service.Run(false);
  
  EXPECT_EQ(result, 0);
  
  // Manually shutdown
  service.Shutdown();
}

// Test: LaunchService run multiple times
TEST(LaunchServiceTest, RunMultipleTimes)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  // First run
  int result1 = service.Run(true);
  EXPECT_EQ(result1, 0);
  
  // Second run should fail (not in idle state)
  int result2 = service.Run(true);
  EXPECT_NE(result2, 0);
}

// Test: LaunchService status queries
TEST(LaunchServiceTest, StatusQueries)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  EXPECT_FALSE(service.IsRunning());
  
  // After run
  service.Run(true);
  
  // Status should not be running anymore
  EXPECT_FALSE(service.IsRunning());
}

// Test: LaunchService with launch arguments
TEST(LaunchServiceTest, WithLaunchArguments)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  auto desc = std::make_shared<LaunchDescription>();
  
  // Add a launch argument declaration
  DeclareLaunchArgument::Options arg_opts;
  arg_opts.name = "test_arg";
  arg_opts.defaultValue = std::make_shared<TextSubstitution>("default_value");
  arg_opts.description = "Test argument";
  desc->Add(std::make_shared<DeclareLaunchArgument>(arg_opts));
  
  Error err = service.IncludeLaunchDescription(desc);
  EXPECT_TRUE(err.IsSuccess());
  
  int result = service.Run(true);
  EXPECT_EQ(result, 0);
}

// Test: LaunchService shutdown
TEST(LaunchServiceTest, Shutdown)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  // Shutdown before run (should be safe)
  service.Shutdown();
  
  // Run then shutdown
  service.Run(false);
  service.Shutdown();
  
  // Multiple shutdowns should be safe
  service.Shutdown();
  service.Shutdown();
}

// Test: LaunchService with multiple launch descriptions
TEST(LaunchServiceTest, MultipleLaunchDescriptions)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  // First description
  auto desc1 = std::make_shared<LaunchDescription>();
  SetLaunchConfiguration::Options opts1;
  opts1.name = "config1";
  opts1.value = std::make_shared<TextSubstitution>("value1");
  desc1->Add(std::make_shared<SetLaunchConfiguration>(opts1));
  
  // Second description
  auto desc2 = std::make_shared<LaunchDescription>();
  SetLaunchConfiguration::Options opts2;
  opts2.name = "config2";
  opts2.value = std::make_shared<TextSubstitution>("value2");
  desc2->Add(std::make_shared<SetLaunchConfiguration>(opts2));
  
  // Include both
  EXPECT_TRUE(service.IncludeLaunchDescription(desc1).IsSuccess());
  EXPECT_TRUE(service.IncludeLaunchDescription(desc2).IsSuccess());
  
  int result = service.Run(true);
  EXPECT_EQ(result, 0);
}

// Test: LaunchService error handling
TEST(LaunchServiceTest, ErrorHandling)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  // Include null description (should fail)
  Error err = service.IncludeLaunchDescription(nullptr);
  EXPECT_TRUE(err.IsError());
}

// Test: LaunchService with empty description
TEST(LaunchServiceTest, EmptyDescription)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  auto desc = std::make_shared<LaunchDescription>();
  // No actions added
  
  Error err = service.IncludeLaunchDescription(desc);
  EXPECT_TRUE(err.IsSuccess());
  
  int result = service.Run(true);
  EXPECT_EQ(result, 0);
}

// Test: LaunchService concurrent access
TEST(LaunchServiceTest, ConcurrentAccess)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  std::atomic<int> include_count(0);
  
  // Multiple threads trying to include descriptions
  std::vector<std::thread> threads;
  for (int i = 0; i < 4; ++i) {
    threads.emplace_back([&service, &include_count, i]() {
      auto desc = std::make_shared<LaunchDescription>();
      SetLaunchConfiguration::Options opts;
      opts.name = "thread_" + std::to_string(i);
      opts.value = std::make_shared<TextSubstitution>("value");
      desc->Add(std::make_shared<SetLaunchConfiguration>(opts));
      
      if (service.IncludeLaunchDescription(desc).IsSuccess()) {
        include_count++;
      }
    });
  }
  
  for (auto& t : threads) {
    t.join();
  }
  
  EXPECT_EQ(include_count.load(), 4);
  
  int result = service.Run(true);
  EXPECT_EQ(result, 0);
}

// Test: LaunchService with complex launch description
TEST(LaunchServiceTest, ComplexLaunchDescription)
{
  LaunchService::Options options;
  LaunchService service(options);
  
  auto desc = std::make_shared<LaunchDescription>();
  
  // Add multiple actions
  for (int i = 0; i < 10; ++i) {
    SetLaunchConfiguration::Options opts;
    opts.name = "config_" + std::to_string(i);
    opts.value = std::make_shared<TextSubstitution>("value_" + std::to_string(i));
    desc->Add(std::make_shared<SetLaunchConfiguration>(opts));
  }
  
  EXPECT_TRUE(service.IncludeLaunchDescription(desc).IsSuccess());
  
  int result = service.Run(true);
  EXPECT_EQ(result, 0);
}

// Test: LaunchService options default values
TEST(LaunchServiceOptionsTest, DefaultValues)
{
  LaunchService::Options options;
  
  EXPECT_TRUE(options.argv.empty());
  EXPECT_FALSE(options.noninteractive);
}

// Test: LaunchService options with values
TEST(LaunchServiceOptionsTest, WithValues)
{
  LaunchService::Options options;
  options.argv = {"arg1", "arg2", "arg3"};
  options.noninteractive = true;
  
  EXPECT_EQ(options.argv.size(), 3U);
  EXPECT_TRUE(options.noninteractive);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
