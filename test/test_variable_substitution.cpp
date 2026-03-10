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
  void SetCurrentLaunchFile(const std::string&) override {}
  std::string GetCurrentLaunchFile() const override { return ""; }

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
  ctx.SetLaunchConfiguration("my_var", "my_value");
  
  VariableSubstitution var("my_var");
  EXPECT_EQ(var.Perform(ctx), "my_value");
}

TEST(VariableSubstitutionTest, DefaultValue)
{
  MockLaunchContext ctx;
  // Don't set the variable
  
  VariableSubstitution var("missing_var", "default_value");
  EXPECT_EQ(var.Perform(ctx), "default_value");
}

TEST(VariableSubstitutionTest, EmptyDefault)
{
  MockLaunchContext ctx;
  // Don't set the variable
  
  VariableSubstitution var("missing_var");
  EXPECT_EQ(var.Perform(ctx), "");
}

TEST(VariableSubstitutionTest, GetVariableName)
{
  VariableSubstitution var("test_var", "default");
  EXPECT_EQ(var.GetVariableName(), "test_var");
}

TEST(VariableSubstitutionTest, GetDefaultValue)
{
  VariableSubstitution var("test_var", "default");
  EXPECT_EQ(var.GetDefaultValue(), "default");
}

TEST(VariableSubstitutionTest, MultipleVariables)
{
  MockLaunchContext ctx;
  ctx.SetLaunchConfiguration("var1", "value1");
  ctx.SetLaunchConfiguration("var2", "value2");
  ctx.SetLaunchConfiguration("var3", "value3");
  
  VariableSubstitution sub1("var1");
  VariableSubstitution sub2("var2");
  VariableSubstitution sub3("var3");
  
  EXPECT_EQ(sub1.Perform(ctx), "value1");
  EXPECT_EQ(sub2.Perform(ctx), "value2");
  EXPECT_EQ(sub3.Perform(ctx), "value3");
}

TEST(VariableSubstitutionTest, OverrideValue)
{
  MockLaunchContext ctx;
  ctx.SetLaunchConfiguration("my_var", "original");
  
  VariableSubstitution var("my_var", "default");
  EXPECT_EQ(var.Perform(ctx), "original");
  
  // Override
  ctx.SetLaunchConfiguration("my_var", "new_value");
  EXPECT_EQ(var.Perform(ctx), "new_value");
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
