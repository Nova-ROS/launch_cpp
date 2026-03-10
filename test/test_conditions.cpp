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


// Comprehensive Conditions Tests
#include <gtest/gtest.h>
#include "launch_cpp/conditions/if_condition.hpp"
#include "launch_cpp/conditions/unless_condition.hpp"
#include "launch_cpp/conditions/launch_configuration_equals.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"
#include "launch_cpp/launch_context.hpp"

using namespace launch_cpp;

// Mock LaunchContext for testing
class MockLaunchContext : public LaunchContext
{
 public:
  void register_event_handler(const EventHandlerPtr&) override {}
  void unregister_event_handler(const EventHandler*) override {}
  const EventHandlerVector& get_event_handlers() const override { return handlers_; }
  void set_launch_configuration(const std::string& key, const std::string& value) override
  {
    configs_[key] = value;
  }
  Result<std::string> get_launch_configuration(const std::string& key) const override
  {
    auto it = configs_.find(key);
    if (it == configs_.end()) {
      return Result<std::string>(Error(ErrorCode::K_INVALID_ARGUMENT, "Not found"));
    }
    return Result<std::string>(it->second);
  }
  bool has_launch_configuration(const std::string& key) const override
  {
    return configs_.find(key) != configs_.end();
  }
  std::string get_environment_variable(const std::string&) const override { return ""; }
  void set_environment_variable(const std::string&, const std::string&) override {}
  void emit_event(EventPtr) override {}
  void set_current_launch_file(const std::string& path) override { currentLaunchFile_ = path; }
  std::string get_current_launch_file() const override { return currentLaunchFile_; }

 private:
  EventHandlerVector handlers_;
  std::unordered_map<std::string, std::string> configs_;
  std::string currentLaunchFile_;
};

// Test: IfCondition with "true"
TEST(IfConditionTest, TrueCondition)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("true"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

// Test: IfCondition with "1"
TEST(IfConditionTest, TrueAsOne)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("1"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

// Test: IfCondition with "yes"
TEST(IfConditionTest, TrueAsYes)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("yes"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

// Test: IfCondition with "false"
TEST(IfConditionTest, FalseCondition)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("false"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

// Test: IfCondition with "0"
TEST(IfConditionTest, FalseAsZero)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("0"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

// Test: IfCondition with "no"
TEST(IfConditionTest, FalseAsNo)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("no"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

// Test: IfCondition with empty string
TEST(IfConditionTest, EmptyString)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>(""));
  EXPECT_FALSE(cond.evaluate(ctx));
}

// Test: UnlessCondition with "true"
TEST(UnlessConditionTest, TrueCondition)
{
  MockLaunchContext ctx;
  UnlessCondition cond(std::make_shared<TextSubstitution>("true"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

// Test: UnlessCondition with "false"
TEST(UnlessConditionTest, FalseCondition)
{
  MockLaunchContext ctx;
  UnlessCondition cond(std::make_shared<TextSubstitution>("false"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

// Test: UnlessCondition with empty string
TEST(UnlessConditionTest, EmptyString)
{
  MockLaunchContext ctx;
  UnlessCondition cond(std::make_shared<TextSubstitution>(""));
  EXPECT_TRUE(cond.evaluate(ctx));
}

// Test: LaunchConfigurationEquals with matching values
TEST(LaunchConfigurationEqualsTest, MatchingValues)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("test_key", "test_value");
  
  LaunchConfigurationEquals cond("test_key", std::make_shared<TextSubstitution>("test_value"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

// Test: LaunchConfigurationEquals with non-matching values
TEST(LaunchConfigurationEqualsTest, NonMatchingValues)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("test_key", "test_value");
  
  LaunchConfigurationEquals cond("test_key", std::make_shared<TextSubstitution>("different_value"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

// Test: LaunchConfigurationEquals with non-existing key
TEST(LaunchConfigurationEqualsTest, NonExistingKey)
{
  MockLaunchContext ctx;
  // Don't set the configuration
  
  LaunchConfigurationEquals cond("non_existing_key", std::make_shared<TextSubstitution>("any_value"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

// Test: LaunchConfigurationEquals with empty expected value
TEST(LaunchConfigurationEqualsTest, EmptyExpectedValue)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("test_key", "");
  
  LaunchConfigurationEquals cond("test_key", std::make_shared<TextSubstitution>(""));
  EXPECT_TRUE(cond.evaluate(ctx));
}

// Test: All conditions implement base interface
TEST(ConditionBaseTest, InterfaceImplementation)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("key", "value");
  
  // Test polymorphic behavior
  std::vector<std::shared_ptr<Condition>> conditions;
  
  conditions.push_back(std::make_shared<IfCondition>(std::make_shared<TextSubstitution>("true")));
  conditions.push_back(std::make_shared<UnlessCondition>(std::make_shared<TextSubstitution>("false")));
  conditions.push_back(std::make_shared<LaunchConfigurationEquals>("key", std::make_shared<TextSubstitution>("value")));
  
  for (auto& cond : conditions) {
    bool result = cond->evaluate(ctx);
    EXPECT_TRUE(result) << "Condition should evaluate to true";
  }
}

// Test: Complex condition scenarios
TEST(ConditionComplexTest, CombinedUsage)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("feature_enabled", "true");
  ctx.set_launch_configuration("mode", "production");
  
  // If feature is enabled
  IfCondition if_cond(std::make_shared<TextSubstitution>("true"));
  
  // Check mode is production
  LaunchConfigurationEquals equals_cond("mode", std::make_shared<TextSubstitution>("production"));
  
  // Unless with falsy value (empty string) should return true
  UnlessCondition unless_cond(std::make_shared<TextSubstitution>(""));
  
  EXPECT_TRUE(if_cond.evaluate(ctx));
  EXPECT_TRUE(equals_cond.evaluate(ctx));
  EXPECT_TRUE(unless_cond.evaluate(ctx));  // empty is falsy, so Unless returns true
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
