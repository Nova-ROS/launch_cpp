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


// Extended Conditions Tests for Low Coverage Files
#include <gtest/gtest.h>
#include <unordered_map>
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
      return Result<std::string>(Error(ErrorCode::kInvalidArgument, "Not found"));
    }
    return Result<std::string>(it->second);
  }
  bool has_launch_configuration(const std::string& key) const override
  {
    return configs_.find(key) != configs_.end();
  }
  std::string get_environment_variable(const std::string& name) const override
  {
    auto it = env_vars_.find(name);
    if (it != env_vars_.end()) {
      return it->second;
    }
    return "";
  }
  void set_environment_variable(const std::string& name, const std::string& value) override
  {
    env_vars_[name] = value;
  }
  void emit_event(EventPtr) override {}
  void set_current_launch_file(const std::string& path) override { currentLaunchFile_ = path; }
  std::string get_current_launch_file() const override { return currentLaunchFile_; }

 private:
  EventHandlerVector handlers_;
  std::unordered_map<std::string, std::string> configs_;
  std::unordered_map<std::string, std::string> env_vars_;
  std::string currentLaunchFile_;
};

// ============================================
// IfCondition Extended Tests
// ============================================

TEST(IfConditionExtendedTest, TrueCondition)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("true"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(IfConditionExtendedTest, FalseCondition)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("false"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

TEST(IfConditionExtendedTest, OneCondition)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("1"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(IfConditionExtendedTest, ZeroCondition)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("0"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

TEST(IfConditionExtendedTest, YesCondition)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("yes"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(IfConditionExtendedTest, NoCondition)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("no"));
  // "no" is not "false" or "0", so it's considered truthy
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(IfConditionExtendedTest, EmptyString)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>(""));
  EXPECT_FALSE(cond.evaluate(ctx));
}

TEST(IfConditionExtendedTest, NullSubstitution)
{
  MockLaunchContext ctx;
  IfCondition cond(nullptr);
  EXPECT_FALSE(cond.evaluate(ctx));
}

TEST(IfConditionExtendedTest, RandomString)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("random_string"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(IfConditionExtendedTest, OnCondition)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("on"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(IfConditionExtendedTest, OffCondition)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("off"));
  // "off" is not "false" or "0", so it's considered truthy
  EXPECT_TRUE(cond.evaluate(ctx));
}

// ============================================
// UnlessCondition Extended Tests
// ============================================

TEST(UnlessConditionExtendedTest, TrueCondition)
{
  MockLaunchContext ctx;
  UnlessCondition cond(std::make_shared<TextSubstitution>("true"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

TEST(UnlessConditionExtendedTest, FalseCondition)
{
  MockLaunchContext ctx;
  UnlessCondition cond(std::make_shared<TextSubstitution>("false"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(UnlessConditionExtendedTest, EmptyString)
{
  MockLaunchContext ctx;
  UnlessCondition cond(std::make_shared<TextSubstitution>(""));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(UnlessConditionExtendedTest, NullSubstitution)
{
  MockLaunchContext ctx;
  UnlessCondition cond(nullptr);
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(UnlessConditionExtendedTest, OneCondition)
{
  MockLaunchContext ctx;
  UnlessCondition cond(std::make_shared<TextSubstitution>("1"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

TEST(UnlessConditionExtendedTest, ZeroCondition)
{
  MockLaunchContext ctx;
  UnlessCondition cond(std::make_shared<TextSubstitution>("0"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(UnlessConditionExtendedTest, YesCondition)
{
  MockLaunchContext ctx;
  UnlessCondition cond(std::make_shared<TextSubstitution>("yes"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

TEST(UnlessConditionExtendedTest, NoCondition)
{
  MockLaunchContext ctx;
  UnlessCondition cond(std::make_shared<TextSubstitution>("no"));
  // "no" is not "false" or "0", so Unless returns false
  EXPECT_FALSE(cond.evaluate(ctx));
}

TEST(UnlessConditionExtendedTest, RandomString)
{
  MockLaunchContext ctx;
  UnlessCondition cond(std::make_shared<TextSubstitution>("random"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

// ============================================
// LaunchConfigurationEquals Extended Tests
// ============================================

TEST(LaunchConfigurationEqualsExtendedTest, MatchingValues)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("test_key", "test_value");
  
  LaunchConfigurationEquals cond("test_key", std::make_shared<TextSubstitution>("test_value"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(LaunchConfigurationEqualsExtendedTest, NonMatchingValues)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("test_key", "test_value");
  
  LaunchConfigurationEquals cond("test_key", std::make_shared<TextSubstitution>("different_value"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

TEST(LaunchConfigurationEqualsExtendedTest, NonExistingKey)
{
  MockLaunchContext ctx;
  LaunchConfigurationEquals cond("non_existing_key", std::make_shared<TextSubstitution>("any_value"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

TEST(LaunchConfigurationEqualsExtendedTest, EmptyExpectedValue)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("test_key", "");
  
  LaunchConfigurationEquals cond("test_key", std::make_shared<TextSubstitution>(""));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(LaunchConfigurationEqualsExtendedTest, NullSubstitution)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("test_key", "test_value");
  
  LaunchConfigurationEquals cond("test_key", nullptr);
  // Should handle null substitution gracefully
  EXPECT_FALSE(cond.evaluate(ctx));
}

TEST(LaunchConfigurationEqualsExtendedTest, EmptyKey)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("", "value");
  
  LaunchConfigurationEquals cond("", std::make_shared<TextSubstitution>("value"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(LaunchConfigurationEqualsExtendedTest, CaseSensitive)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("key", "Value");
  
  LaunchConfigurationEquals cond("key", std::make_shared<TextSubstitution>("value"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

TEST(LaunchConfigurationEqualsExtendedTest, MultipleConfigs)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("key1", "value1");
  ctx.set_launch_configuration("key2", "value2");
  
  LaunchConfigurationEquals cond1("key1", std::make_shared<TextSubstitution>("value1"));
  LaunchConfigurationEquals cond2("key2", std::make_shared<TextSubstitution>("value2"));
  
  EXPECT_TRUE(cond1.evaluate(ctx));
  EXPECT_TRUE(cond2.evaluate(ctx));
}

// ============================================
// Complex Condition Tests
// ============================================

TEST(ConditionComplexTest, CombinedUsage)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("mode", "production");
  ctx.set_launch_configuration("debug", "false");
  
  // Test: mode == "production" AND debug == "false"
  LaunchConfigurationEquals mode_cond("mode", std::make_shared<TextSubstitution>("production"));
  LaunchConfigurationEquals debug_cond("debug", std::make_shared<TextSubstitution>("false"));
  
  EXPECT_TRUE(mode_cond.evaluate(ctx));
  EXPECT_TRUE(debug_cond.evaluate(ctx));
}

TEST(ConditionComplexTest, IfWithConfig)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("enabled", "true");
  
  IfCondition if_cond(std::make_shared<TextSubstitution>("true"));
  LaunchConfigurationEquals equals_cond("enabled", std::make_shared<TextSubstitution>("true"));
  
  EXPECT_TRUE(if_cond.evaluate(ctx));
  EXPECT_TRUE(equals_cond.evaluate(ctx));
}

TEST(ConditionComplexTest, UnlessWithConfig)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("disabled", "false");
  
  UnlessCondition unless_cond(std::make_shared<TextSubstitution>("false"));
  LaunchConfigurationEquals equals_cond("disabled", std::make_shared<TextSubstitution>("false"));
  
  EXPECT_TRUE(unless_cond.evaluate(ctx));
  EXPECT_TRUE(equals_cond.evaluate(ctx));
}

TEST(ConditionComplexTest, AllConditionsTogether)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("key1", "value1");
  
  IfCondition if_true(std::make_shared<TextSubstitution>("true"));
  UnlessCondition unless_false(std::make_shared<TextSubstitution>("false"));
  LaunchConfigurationEquals equals("key1", std::make_shared<TextSubstitution>("value1"));
  
  EXPECT_TRUE(if_true.evaluate(ctx));
  EXPECT_TRUE(unless_false.evaluate(ctx));
  EXPECT_TRUE(equals.evaluate(ctx));
}

// ============================================
// Edge Cases
// ============================================

TEST(ConditionEdgeCaseTest, WhitespaceInString)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("  true  "));
  // Whitespace makes it not exactly "true", so it's truthy
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(ConditionEdgeCaseTest, SpecialCharacters)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>("!@#$%"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(ConditionEdgeCaseTest, LongString)
{
  MockLaunchContext ctx;
  IfCondition cond(std::make_shared<TextSubstitution>(std::string(1000, 'a')));
  EXPECT_TRUE(cond.evaluate(ctx));
}

TEST(ConditionEdgeCaseTest, NumericValues)
{
  MockLaunchContext ctx;
  
  IfCondition cond1(std::make_shared<TextSubstitution>("123"));
  IfCondition cond2(std::make_shared<TextSubstitution>("-1"));
  IfCondition cond3(std::make_shared<TextSubstitution>("0"));
  
  EXPECT_TRUE(cond1.evaluate(ctx));
  EXPECT_TRUE(cond2.evaluate(ctx));
  EXPECT_FALSE(cond3.evaluate(ctx));
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
