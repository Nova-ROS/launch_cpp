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

/**
 * @file test_variable_substitution.cpp
 * @brief Unit tests for VariableSubstitution
 */

#include <gtest/gtest.h>
#include "launch_cpp/substitutions/variable_substitution.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/event.hpp"
#include "launch_cpp/event_handler.hpp"
#include "launch_cpp/error_code.hpp"

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
  std::string get_environment_variable(const std::string&) const override { return ""; }
  void set_environment_variable(const std::string&, const std::string&) override {}
  void emit_event(EventPtr) override {}
  void set_current_launch_file(const std::string&) override {}
  std::string get_current_launch_file() const override { return ""; }

 private:
  EventHandlerVector handlers_;
  std::unordered_map<std::string, std::string> configs_;
};

// ============================================================================
// VariableSubstitution Tests
// ============================================================================

TEST(VariableSubstitutionTest, BasicSubstitution)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("my_var", "my_value");
  
  VariableSubstitution var("my_var");
  EXPECT_EQ(var.perform(ctx), "my_value");
}

TEST(VariableSubstitutionTest, DefaultValue)
{
  MockLaunchContext ctx;
  // Don't set the variable
  
  VariableSubstitution var("missing_var", "default_value");
  EXPECT_EQ(var.perform(ctx), "default_value");
}

TEST(VariableSubstitutionTest, EmptyDefault)
{
  MockLaunchContext ctx;
  // Don't set the variable
  
  VariableSubstitution var("missing_var");
  EXPECT_EQ(var.perform(ctx), "");
}

TEST(VariableSubstitutionTest, GetVariableName)
{
  VariableSubstitution var("test_var", "default");
  EXPECT_EQ(var.get_variable_name(), "test_var");
}

TEST(VariableSubstitutionTest, GetDefaultValue)
{
  VariableSubstitution var("test_var", "default");
  EXPECT_EQ(var.get_default_value(), "default");
}

TEST(VariableSubstitutionTest, MultipleVariables)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("var1", "value1");
  ctx.set_launch_configuration("var2", "value2");
  ctx.set_launch_configuration("var3", "value3");
  
  VariableSubstitution sub1("var1");
  VariableSubstitution sub2("var2");
  VariableSubstitution sub3("var3");
  
  EXPECT_EQ(sub1.perform(ctx), "value1");
  EXPECT_EQ(sub2.perform(ctx), "value2");
  EXPECT_EQ(sub3.perform(ctx), "value3");
}

TEST(VariableSubstitutionTest, OverrideValue)
{
  MockLaunchContext ctx;
  ctx.set_launch_configuration("my_var", "original");
  
  VariableSubstitution var("my_var", "default");
  EXPECT_EQ(var.perform(ctx), "original");
  
  // Override
  ctx.set_launch_configuration("my_var", "new_value");
  EXPECT_EQ(var.perform(ctx), "new_value");
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
