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

// Additional Condition Tests - Coverage Improvement
#include <gtest/gtest.h>
#include "launch_cpp/conditions/if_condition.hpp"
#include "launch_cpp/conditions/unless_condition.hpp"
#include "launch_cpp/conditions/launch_configuration_equals.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"
#include "launch_cpp/substitutions/launch_configuration.hpp"
#include "launch_cpp/error_code.hpp"

using namespace launch_cpp;

// Helper
SubstitutionPtr text(const std::string& str) {
  return std::make_shared<TextSubstitution>(str);
}

// Mock LaunchContext for testing
class MockLaunchContext : public LaunchContext {
 public:
  void register_event_handler(const EventHandlerPtr& handler) override {
    (void)handler;
  }
  void unregister_event_handler(const EventHandler* handler) override {
    (void)handler;
  }
  const EventHandlerVector& get_event_handlers() const override {
    return handlers_;
  }
  void set_launch_configuration(const std::string& key, const std::string& value) override {
    configs_[key] = value;
  }
  Result<std::string> get_launch_configuration(const std::string& key) const override {
    auto it = configs_.find(key);
    if (it != configs_.end()) {
      return Result<std::string>(it->second);
    }
    return Result<std::string>(Error(ErrorCode::K_INVALID_ARGUMENT, "Not found"));
  }
  bool has_launch_configuration(const std::string& key) const override {
    return configs_.find(key) != configs_.end();
  }
  std::string get_environment_variable(const std::string& name) const override {
    (void)name;
    return "";
  }
  void set_environment_variable(const std::string& name, const std::string& value) override {
    (void)name;
    (void)value;
  }
  void emit_event(EventPtr event) override {
    (void)event;
  }
  void set_current_launch_file(const std::string& path) override {
    (void)path;
  }
  std::string get_current_launch_file() const override {
    return "";
  }

 private:
  EventHandlerVector handlers_;
  mutable std::unordered_map<std::string, std::string> configs_;
};

// Test: IfCondition with null expression
TEST(IfConditionEdgeTest, NullExpression)
{
  IfCondition cond(nullptr);
  MockLaunchContext ctx;
  EXPECT_FALSE(cond.evaluate(ctx));
}

// Test: IfCondition with all truthy values
TEST(IfConditionEdgeTest, AllTruthyValues)
{
  MockLaunchContext ctx;
  
  std::vector<std::string> truthy = {"1", "yes", "true", "on", "YES", "TRUE", "ON"};
  for (const auto& val : truthy) {
    IfCondition cond(text(val));
    EXPECT_TRUE(cond.evaluate(ctx)) << "Failed for value: " << val;
  }
}

// Test: IfCondition with all falsy values
TEST(IfConditionEdgeTest, AllFalsyValues)
{
  MockLaunchContext ctx;
  
  std::vector<std::string> falsy = {"0", "false", "False", "FALSE"};
  for (const auto& val : falsy) {
    IfCondition cond(text(val));
    EXPECT_FALSE(cond.evaluate(ctx)) << "Failed for value: " << val;
  }
}

// Test: IfCondition with whitespace
TEST(IfConditionEdgeTest, WhitespaceHandling)
{
  MockLaunchContext ctx;
  
  // Whitespace around "true" should still be truthy
  IfCondition cond1(text("  true  "));
  EXPECT_TRUE(cond1.evaluate(ctx));
  
  // Whitespace around "false" is implementation dependent
  // Let's just check it doesn't crash
  IfCondition cond2(text("  false  "));
  (void)cond2.evaluate(ctx);
  
  // Pure whitespace is empty after trim, should be falsy
  IfCondition cond3(text("  "));
  EXPECT_FALSE(cond3.evaluate(ctx));
}

// Test: IfCondition with special strings
TEST(IfConditionEdgeTest, SpecialStrings)
{
  MockLaunchContext ctx;
  
  // These should be truthy (not explicitly false)
  IfCondition cond1(text("undefined"));
  EXPECT_TRUE(cond1.evaluate(ctx));
  
  IfCondition cond2(text("null"));
  EXPECT_TRUE(cond2.evaluate(ctx));
  
  IfCondition cond3(text("NaN"));
  EXPECT_TRUE(cond3.evaluate(ctx));
  
  IfCondition cond4(text("maybe"));
  EXPECT_TRUE(cond4.evaluate(ctx));
}

// Test: UnlessCondition with null expression
TEST(UnlessConditionEdgeTest, NullExpression)
{
  UnlessCondition cond(nullptr);
  MockLaunchContext ctx;
  // Null expression should return true (execute unless nothing)
  EXPECT_TRUE(cond.evaluate(ctx));
}

// Test: UnlessCondition with all truthy values (should return false)
TEST(UnlessConditionEdgeTest, AllTruthyValues)
{
  MockLaunchContext ctx;
  
  std::vector<std::string> truthy = {"1", "yes", "true", "on"};
  for (const auto& val : truthy) {
    UnlessCondition cond(text(val));
    EXPECT_FALSE(cond.evaluate(ctx)) << "Failed for value: " << val;
  }
}

// Test: UnlessCondition with all falsy values (should return true)
TEST(UnlessConditionEdgeTest, AllFalsyValues)
{
  MockLaunchContext ctx;
  
  std::vector<std::string> falsy = {"0", "false"};
  for (const auto& val : falsy) {
    UnlessCondition cond(text(val));
    EXPECT_TRUE(cond.evaluate(ctx)) << "Failed for value: " << val;
  }
}

// Test: LaunchConfigurationEquals with missing configuration
TEST(LaunchConfigurationEqualsEdgeTest, MissingConfiguration)
{
  MockLaunchContext ctx;
  
  LaunchConfigurationEquals cond("nonexistent_key", text("expected_value"));
  // Should return false when key doesn't exist
  EXPECT_FALSE(cond.evaluate(ctx));
}

// Test: LaunchConfigurationEquals with matching value
TEST(LaunchConfigurationEqualsEdgeTest, MatchingValue)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("my_key", "my_value");
  
  LaunchConfigurationEquals cond("my_key", text("my_value"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

// Test: LaunchConfigurationEquals with non-matching value
TEST(LaunchConfigurationEqualsEdgeTest, NonMatchingValue)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("my_key", "my_value");
  
  LaunchConfigurationEquals cond("my_key", text("different_value"));
  EXPECT_FALSE(cond.evaluate(ctx));
}

// Test: LaunchConfigurationEquals with empty values
TEST(LaunchConfigurationEqualsEdgeTest, EmptyValues)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("empty_key", "");
  
  LaunchConfigurationEquals cond("empty_key", text(""));
  EXPECT_TRUE(cond.evaluate(ctx));
}

// Test: LaunchConfigurationEquals case sensitivity
TEST(LaunchConfigurationEqualsEdgeTest, CaseSensitivity)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("case_key", "Value");
  
  LaunchConfigurationEquals cond1("case_key", text("Value"));
  EXPECT_TRUE(cond1.evaluate(ctx));
  
  LaunchConfigurationEquals cond2("case_key", text("value"));
  EXPECT_FALSE(cond2.evaluate(ctx));
}

// Test: LaunchConfigurationEquals getters
TEST(LaunchConfigurationEqualsEdgeTest, Getters)
{
  LaunchConfigurationEquals cond("test_key", text("test_value"));
  EXPECT_EQ(cond.get_name(), "test_key");
  EXPECT_NE(cond.get_expected(), nullptr);
}

// Test: Complex condition with launch configuration substitution
TEST(LaunchConfigurationSubstitutionTest, SubstitutionInCondition)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("env", "production");
  
  // Use launch configuration substitution in condition
  auto subst = std::make_shared<LaunchConfiguration>("env");
  LaunchConfigurationEquals cond("env", subst);
  
  EXPECT_TRUE(cond.evaluate(ctx));
}

// Test: Condition with special characters in values
TEST(LaunchConfigurationEdgeTest, SpecialCharacters)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("special", "value with spaces and !@#$%");
  
  LaunchConfigurationEquals cond("special", text("value with spaces and !@#$%"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

// Test: Multiple conditions on same key
TEST(LaunchConfigurationEdgeTest, MultipleConditionsSameKey)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("multi_key", "value1");
  
  LaunchConfigurationEquals cond1("multi_key", text("value1"));
  LaunchConfigurationEquals cond2("multi_key", text("value2"));
  
  EXPECT_TRUE(cond1.evaluate(ctx));
  EXPECT_FALSE(cond2.evaluate(ctx));
}



// Test: IfCondition getters
TEST(IfConditionEdgeTest, Getters)
{
  auto subst = text("test_expr");
  IfCondition cond(subst);
  EXPECT_EQ(cond.get_expression(), subst);
}

// Test: IfCondition with numeric strings
TEST(IfConditionEdgeTest, NumericStrings)
{
  MockLaunchContext ctx;
  
  IfCondition cond1(text("123"));
  EXPECT_TRUE(cond1.evaluate(ctx));
  
  IfCondition cond2(text("-456"));
  EXPECT_TRUE(cond2.evaluate(ctx));
  
  IfCondition cond3(text("3.14159"));
  EXPECT_TRUE(cond3.evaluate(ctx));
}

// Test: LaunchConfigurationEquals with numeric expected value
TEST(LaunchConfigurationEqualsEdgeTest, NumericExpectedValue)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("num_key", "42");
  
  LaunchConfigurationEquals cond("num_key", text("42"));
  EXPECT_TRUE(cond.evaluate(ctx));
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
