// Copyright 2024 Example Author
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

// Comprehensive Actions Tests
#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "cpp_launch/action.hpp"
#include "cpp_launch/actions/execute_process.hpp"
#include "cpp_launch/actions/declare_launch_argument.hpp"
#include "cpp_launch/actions/set_launch_configuration.hpp"
#include "cpp_launch/actions/timer_action.hpp"
#include "cpp_launch/actions/group_action.hpp"
#include "cpp_launch/actions/include_launch_description.hpp"
#include "cpp_launch/launch_context.hpp"
#include "cpp_launch/conditions/if_condition.hpp"
#include "cpp_launch/substitutions/text_substitution.hpp"

using namespace cpp_launch;

// Mock LaunchContext for testing
class MockLaunchContext : public LaunchContext
{
 public:
  void RegisterEventHandler(const EventHandlerPtr&) override {}
  void UnregisterEventHandler(const EventHandler*) override {}
  const EventHandlerVector& GetEventHandlers() const override { return handlers_; }
  void SetLaunchConfiguration(const std::string& key, const std::string& value) override
  {
    configs_[key] = value;
  }
  Result<std::string> GetLaunchConfiguration(const std::string& key) const override
  {
    auto it = configs_.find(key);
    if (it == configs_.end()) {
      return Result<std::string>(Error(ErrorCode::kInvalidArgument, "Not found"));
    }
    return Result<std::string>(it->second);
  }
  bool HasLaunchConfiguration(const std::string& key) const override
  {
    return configs_.find(key) != configs_.end();
  }
  std::string GetEnvironmentVariable(const std::string&) const override { return ""; }
  void SetEnvironmentVariable(const std::string&, const std::string&) override {}
  void EmitEvent(EventPtr) override {}
  void SetCurrentLaunchFile(const std::string& path) override { currentLaunchFile_ = path; }
  std::string GetCurrentLaunchFile() const override { return currentLaunchFile_; }

 private:
  EventHandlerVector handlers_;
  std::unordered_map<std::string, std::string> configs_;
  std::string currentLaunchFile_;
};

// Test: ExecuteProcess basic creation
TEST(ExecuteProcessTest, BasicCreation)
{
  ExecuteProcess::Options options;
  options.cmd = {std::make_shared<TextSubstitution>("echo"), std::make_shared<TextSubstitution>("hello")};
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
}

// Test: ExecuteProcess with environment variables
TEST(ExecuteProcessTest, WithEnvironment)
{
  ExecuteProcess::Options options;
  options.cmd = {std::make_shared<TextSubstitution>("echo"), std::make_shared<TextSubstitution>("test")};
  options.env = {{"VAR1", std::make_shared<TextSubstitution>("value1")}, {"VAR2", std::make_shared<TextSubstitution>("value2")}};
  
  auto action = std::make_shared<ExecuteProcess>(options);
  // Just verify it doesn't crash - GetEnvironment is not exposed
  EXPECT_NE(action, nullptr);
}

// Test: ExecuteProcess with cwd
TEST(ExecuteProcessTest, WithWorkingDirectory)
{
  ExecuteProcess::Options options;
  options.cmd = {std::make_shared<TextSubstitution>("pwd")};
  options.cwd = std::make_shared<TextSubstitution>("/tmp");
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
}

// Test: DeclareLaunchArgument basic creation
TEST(DeclareLaunchArgumentTest, BasicCreation)
{
  DeclareLaunchArgument::Options options;
  options.name = "test_arg";
  options.defaultValue = std::make_shared<TextSubstitution>("default_value");
  options.description = "Test argument description";
  
  auto action = std::make_shared<DeclareLaunchArgument>(options);
  EXPECT_EQ(action->GetName(), "test_arg");
  EXPECT_NE(action->GetDefaultValue(), nullptr);
}

// Test: DeclareLaunchArgument execute
TEST(DeclareLaunchArgumentTest, Execute)
{
  MockLaunchContext ctx;
  DeclareLaunchArgument::Options options;
  options.name = "my_arg";
  options.defaultValue = std::make_shared<TextSubstitution>("default_val");
  
  auto action = std::make_shared<DeclareLaunchArgument>(options);
  auto result = action->Execute(ctx);
  
  EXPECT_TRUE(result.HasValue());
  EXPECT_TRUE(ctx.HasLaunchConfiguration("my_arg"));
  
  auto val_result = ctx.GetLaunchConfiguration("my_arg");
  EXPECT_TRUE(val_result.HasValue());
  EXPECT_EQ(val_result.GetValue(), "default_val");
}

// Test: SetLaunchConfiguration basic creation
TEST(SetLaunchConfigurationTest, BasicCreation)
{
  SetLaunchConfiguration::Options options;
  options.name = "config_key";
  options.value = std::make_shared<TextSubstitution>("config_value");
  
  auto action = std::make_shared<SetLaunchConfiguration>(options);
  EXPECT_EQ(action->GetName(), "config_key");
}

// Test: SetLaunchConfiguration execute
TEST(SetLaunchConfigurationTest, Execute)
{
  MockLaunchContext ctx;
  SetLaunchConfiguration::Options options;
  options.name = "my_config";
  options.value = std::make_shared<TextSubstitution>("my_value");
  
  auto action = std::make_shared<SetLaunchConfiguration>(options);
  auto result = action->Execute(ctx);
  
  EXPECT_TRUE(result.HasValue());
  EXPECT_TRUE(ctx.HasLaunchConfiguration("my_config"));
  
  auto val_result = ctx.GetLaunchConfiguration("my_config");
  EXPECT_TRUE(val_result.HasValue());
  EXPECT_EQ(val_result.GetValue(), "my_value");
}

// Test: SetLaunchConfiguration with null substitution
TEST(SetLaunchConfigurationTest, ExecuteWithNullSubstitution)
{
  MockLaunchContext ctx;
  SetLaunchConfiguration::Options options;
  options.name = "empty_config";
  options.value = nullptr;
  
  auto action = std::make_shared<SetLaunchConfiguration>(options);
  auto result = action->Execute(ctx);
  
  EXPECT_TRUE(result.HasValue());
  EXPECT_TRUE(ctx.HasLaunchConfiguration("empty_config"));
  
  auto val_result = ctx.GetLaunchConfiguration("empty_config");
  EXPECT_TRUE(val_result.HasValue());
  EXPECT_EQ(val_result.GetValue(), "");
}

// Test: TimerAction basic creation
TEST(TimerActionTest, BasicCreation)
{
  TimerAction::Options options;
  options.period = std::chrono::milliseconds(100);
  
  auto action = std::make_shared<TimerAction>(options);
  EXPECT_EQ(action->GetPeriod(), std::chrono::milliseconds(100));
  EXPECT_EQ(action->GetActions().size(), 0U);
}

// Test: TimerAction with nested actions
TEST(TimerActionTest, WithNestedActions)
{
  TimerAction::Options options;
  options.period = std::chrono::milliseconds(50);
  
  // Add some nested actions
  SetLaunchConfiguration::Options set_opts;
  set_opts.name = "timer_test";
  set_opts.value = std::make_shared<TextSubstitution>("value");
  options.actions.push_back(std::make_shared<SetLaunchConfiguration>(set_opts));
  
  auto action = std::make_shared<TimerAction>(options);
  EXPECT_EQ(action->GetActions().size(), 1U);
}

// Test: TimerAction execute (timing test)
TEST(TimerActionTest, ExecuteTiming)
{
  MockLaunchContext ctx;
  TimerAction::Options options;
  options.period = std::chrono::milliseconds(100);
  
  auto action = std::make_shared<TimerAction>(options);
  
  auto start = std::chrono::steady_clock::now();
  auto result = action->Execute(ctx);
  auto end = std::chrono::steady_clock::now();
  
  EXPECT_TRUE(result.HasValue());
  
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  EXPECT_GE(elapsed.count(), 90);  // Should wait at least 90ms (with some tolerance)
}

// Test: GroupAction basic creation
TEST(GroupActionTest, BasicCreation)
{
  GroupAction::Options options;
  
  auto action = std::make_shared<GroupAction>(options);
  EXPECT_EQ(action->GetActions().size(), 0U);
  EXPECT_EQ(action->GetCondition(), nullptr);
}

// Test: GroupAction with nested actions
TEST(GroupActionTest, WithNestedActions)
{
  GroupAction::Options options;
  
  // Add some nested actions
  SetLaunchConfiguration::Options set_opts1;
  set_opts1.name = "group_test1";
  set_opts1.value = std::make_shared<TextSubstitution>("value1");
  options.actions.push_back(std::make_shared<SetLaunchConfiguration>(set_opts1));
  
  SetLaunchConfiguration::Options set_opts2;
  set_opts2.name = "group_test2";
  set_opts2.value = std::make_shared<TextSubstitution>("value2");
  options.actions.push_back(std::make_shared<SetLaunchConfiguration>(set_opts2));
  
  auto action = std::make_shared<GroupAction>(options);
  EXPECT_EQ(action->GetActions().size(), 2U);
}

// Test: GroupAction execute without condition
TEST(GroupActionTest, ExecuteWithoutCondition)
{
  MockLaunchContext ctx;
  GroupAction::Options options;
  
  // Add nested action
  SetLaunchConfiguration::Options set_opts;
  set_opts.name = "group_config";
  set_opts.value = std::make_shared<TextSubstitution>("group_value");
  options.actions.push_back(std::make_shared<SetLaunchConfiguration>(set_opts));
  
  auto action = std::make_shared<GroupAction>(options);
  auto result = action->Execute(ctx);
  
  EXPECT_TRUE(result.HasValue());
  EXPECT_TRUE(ctx.HasLaunchConfiguration("group_config"));
}

// Test: GroupAction execute with condition
TEST(GroupActionTest, ExecuteWithCondition)
{
  MockLaunchContext ctx;
  GroupAction::Options options;
  
  // Add nested action
  SetLaunchConfiguration::Options set_opts;
  set_opts.name = "conditional_config";
  set_opts.value = std::make_shared<TextSubstitution>("conditional_value");
  options.actions.push_back(std::make_shared<SetLaunchConfiguration>(set_opts));
  
  // Add condition that evaluates to true
  options.condition = std::make_shared<IfCondition>(std::make_shared<TextSubstitution>("true"));
  
  auto action = std::make_shared<GroupAction>(options);
  auto result = action->Execute(ctx);
  
  EXPECT_TRUE(result.HasValue());
  EXPECT_TRUE(ctx.HasLaunchConfiguration("conditional_config"));
}

// Test: GroupAction execute with failing condition
TEST(GroupActionTest, ExecuteWithFailingCondition)
{
  MockLaunchContext ctx;
  GroupAction::Options options;
  
  // Add nested action
  SetLaunchConfiguration::Options set_opts;
  set_opts.name = "should_not_exist";
  set_opts.value = std::make_shared<TextSubstitution>("value");
  options.actions.push_back(std::make_shared<SetLaunchConfiguration>(set_opts));
  
  // Add condition that evaluates to false
  options.condition = std::make_shared<IfCondition>(std::make_shared<TextSubstitution>("false"));
  
  auto action = std::make_shared<GroupAction>(options);
  auto result = action->Execute(ctx);
  
  EXPECT_TRUE(result.HasValue());
  EXPECT_FALSE(ctx.HasLaunchConfiguration("should_not_exist"));
}

// Test: IncludeLaunchDescription basic creation
TEST(IncludeLaunchDescriptionTest, BasicCreation)
{
  IncludeLaunchDescription::Options options;
  options.launchDescriptionSource = std::make_shared<TextSubstitution>("/path/to/launch.yaml");
  
  auto action = std::make_shared<IncludeLaunchDescription>(options);
  // Just verify it doesn't crash
  EXPECT_NE(action, nullptr);
}

// Test: Action base class functionality
TEST(ActionBaseTest, GetCondition)
{
  ExecuteProcess::Options options;
  options.cmd = {std::make_shared<TextSubstitution>("echo"), std::make_shared<TextSubstitution>("test")};
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_EQ(action->GetCondition(), nullptr);
}

// Test: Action with condition
TEST(ActionBaseTest, WithCondition)
{
  ExecuteProcess::Options options;
  options.cmd = {std::make_shared<TextSubstitution>("echo"), std::make_shared<TextSubstitution>("test")};
  
  auto action = std::make_shared<ExecuteProcess>(options);
  // Action doesn't have condition field in Options, it's set via base class
  EXPECT_EQ(action->GetCondition(), nullptr);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
