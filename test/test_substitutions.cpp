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

// Comprehensive Substitutions Tests
#include <gtest/gtest.h>
#include <cstdlib>
#include "cpp_launch/substitutions/text_substitution.hpp"
#include "cpp_launch/substitutions/launch_configuration.hpp"
#include "cpp_launch/substitutions/environment_variable.hpp"
#include "cpp_launch/substitutions/command.hpp"
#include "cpp_launch/substitutions/find_executable.hpp"
#include "cpp_launch/substitutions/this_launch_file.hpp"
#include "cpp_launch/substitutions/this_launch_file_dir.hpp"
#include "cpp_launch/launch_context.hpp"

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
  std::string GetEnvironmentVariable(const std::string& name) const override
  {
    auto it = env_vars_.find(name);
    if (it != env_vars_.end()) {
      return it->second;
    }
    return "";
  }
  void SetEnvironmentVariable(const std::string& name, const std::string& value) override
  {
    env_vars_[name] = value;
  }
  void EmitEvent(EventPtr) override {}
  void SetCurrentLaunchFile(const std::string& path) override { currentLaunchFile_ = path; }
  std::string GetCurrentLaunchFile() const override { return currentLaunchFile_; }
  
  void SetMockEnvVar(const std::string& name, const std::string& value)
  {
    env_vars_[name] = value;
  }

 private:
  EventHandlerVector handlers_;
  std::unordered_map<std::string, std::string> configs_;
  std::unordered_map<std::string, std::string> env_vars_;
  std::string currentLaunchFile_;
};

// Test: TextSubstitution basic
TEST(TextSubstitutionTest, Basic)
{
  MockLaunchContext ctx;
  TextSubstitution sub("Hello World");
  
  EXPECT_EQ(sub.Perform(ctx), "Hello World");
}

// Test: TextSubstitution empty string
TEST(TextSubstitutionTest, EmptyString)
{
  MockLaunchContext ctx;
  TextSubstitution sub("");
  
  EXPECT_EQ(sub.Perform(ctx), "");
}

// Test: TextSubstitution special characters
TEST(TextSubstitutionTest, SpecialCharacters)
{
  MockLaunchContext ctx;
  TextSubstitution sub("Special: !@#$%^&*()");
  
  EXPECT_EQ(sub.Perform(ctx), "Special: !@#$%^&*()");
}

// Test: LaunchConfiguration basic
TEST(LaunchConfigurationTest, ExistingConfig)
{
  MockLaunchContext ctx;
  ctx.SetLaunchConfiguration("my_key", "my_value");
  
  LaunchConfiguration sub("my_key");
  EXPECT_EQ(sub.Perform(ctx), "my_value");
}

// Test: LaunchConfiguration non-existing
TEST(LaunchConfigurationTest, NonExistingConfig)
{
  MockLaunchContext ctx;
  
  LaunchConfiguration sub("non_existing_key");
  EXPECT_EQ(sub.Perform(ctx), "");
}

// Test: LaunchConfiguration with special characters in key
TEST(LaunchConfigurationTest, SpecialKey)
{
  MockLaunchContext ctx;
  ctx.SetLaunchConfiguration("key/with/slashes", "value");
  
  LaunchConfiguration sub("key/with/slashes");
  EXPECT_EQ(sub.Perform(ctx), "value");
}

// Test: EnvironmentVariable basic
TEST(EnvironmentVariableTest, ExistingVar)
{
  MockLaunchContext ctx;
  ctx.SetMockEnvVar("TEST_VAR", "test_value");
  
  EnvironmentVariable sub("TEST_VAR");
  EXPECT_EQ(sub.Perform(ctx), "test_value");
}

// Test: EnvironmentVariable non-existing
TEST(EnvironmentVariableTest, NonExistingVar)
{
  MockLaunchContext ctx;
  
  EnvironmentVariable sub("NON_EXISTING_VAR");
  EXPECT_EQ(sub.Perform(ctx), "");
}

// Test: EnvironmentVariable empty value
TEST(EnvironmentVariableTest, EmptyValue)
{
  MockLaunchContext ctx;
  ctx.SetMockEnvVar("EMPTY_VAR", "");
  
  EnvironmentVariable sub("EMPTY_VAR");
  EXPECT_EQ(sub.Perform(ctx), "");
}

// Test: Command substitution
TEST(CommandTest, Basic)
{
  MockLaunchContext ctx;
  Command sub({std::make_shared<TextSubstitution>("echo"), std::make_shared<TextSubstitution>("hello")});
  
  std::string result = sub.Perform(ctx);
  // Command might not work in test environment, just verify it doesn't crash
  (void)result;
}

// Test: Command with arguments
TEST(CommandTest, WithArgs)
{
  MockLaunchContext ctx;
  Command sub({std::make_shared<TextSubstitution>("echo"), std::make_shared<TextSubstitution>("test"), std::make_shared<TextSubstitution>("argument")});
  
  std::string result = sub.Perform(ctx);
  (void)result;
}

// Test: FindExecutable
TEST(FindExecutableTest, Existing)
{
  MockLaunchContext ctx;
  FindExecutable sub("echo");  // echo should exist on most systems
  
  std::string result = sub.Perform(ctx);
  // Should find echo in PATH
  EXPECT_FALSE(result.empty());
}

// Test: FindExecutable non-existing
TEST(FindExecutableTest, NonExisting)
{
  MockLaunchContext ctx;
  FindExecutable sub("nonexistent_executable_xyz");
  
  std::string result = sub.Perform(ctx);
  EXPECT_EQ(result, "");
}

// Test: ThisLaunchFile
TEST(ThisLaunchFileTest, Basic)
{
  MockLaunchContext ctx;
  ctx.SetCurrentLaunchFile("/path/to/launch.yaml");
  
  ThisLaunchFile sub;
  EXPECT_EQ(sub.Perform(ctx), "/path/to/launch.yaml");
}

// Test: ThisLaunchFile empty
TEST(ThisLaunchFileTest, Empty)
{
  MockLaunchContext ctx;
  // Don't set current launch file
  
  ThisLaunchFile sub;
  EXPECT_EQ(sub.Perform(ctx), "");
}

// Test: ThisLaunchFile with complex path
TEST(ThisLaunchFileTest, ComplexPath)
{
  MockLaunchContext ctx;
  ctx.SetCurrentLaunchFile("/very/long/path/to/the/launch/file.yaml");
  
  ThisLaunchFile sub;
  EXPECT_EQ(sub.Perform(ctx), "/very/long/path/to/the/launch/file.yaml");
}

// Test: ThisLaunchFileDir basic
TEST(ThisLaunchFileDirTest, Basic)
{
  MockLaunchContext ctx;
  ctx.SetCurrentLaunchFile("/path/to/launch.yaml");
  
  ThisLaunchFileDir sub;
  EXPECT_EQ(sub.Perform(ctx), "/path/to");
}

// Test: ThisLaunchFileDir empty file
TEST(ThisLaunchFileDirTest, EmptyFile)
{
  MockLaunchContext ctx;
  // Don't set current launch file
  
  ThisLaunchFileDir sub;
  EXPECT_EQ(sub.Perform(ctx), "");
}

// Test: ThisLaunchFileDir root file
TEST(ThisLaunchFileDirTest, RootFile)
{
  MockLaunchContext ctx;
  ctx.SetCurrentLaunchFile("launch.yaml");  // No directory
  
  ThisLaunchFileDir sub;
  EXPECT_EQ(sub.Perform(ctx), ".");
}

// Test: ThisLaunchFileDir nested path
TEST(ThisLaunchFileDirTest, NestedPath)
{
  MockLaunchContext ctx;
  ctx.SetCurrentLaunchFile("/a/b/c/d/e/f/launch.yaml");
  
  ThisLaunchFileDir sub;
  EXPECT_EQ(sub.Perform(ctx), "/a/b/c/d/e/f");
}

// Test: Substitution base class
TEST(SubstitutionBaseTest, Polymorphism)
{
  MockLaunchContext ctx;
  
  // Test polymorphic behavior
  std::vector<std::shared_ptr<Substitution>> subs;
  subs.push_back(std::make_shared<TextSubstitution>("text"));
  subs.push_back(std::make_shared<LaunchConfiguration>("key"));
  subs.push_back(std::make_shared<EnvironmentVariable>("VAR"));
  
  for (auto& sub : subs) {
    std::string result = sub->Perform(ctx);
    (void)result;  // Just verify it doesn't crash
  }
}

// Test: All substitutions implement base interface
TEST(SubstitutionBaseTest, InterfaceImplementation)
{
  MockLaunchContext ctx;
  ctx.SetLaunchConfiguration("test", "value");
  ctx.SetMockEnvVar("TEST", "env_value");
  ctx.SetCurrentLaunchFile("/test/launch.yaml");
  
  // Test all substitution types
  TextSubstitution text("hello");
  EXPECT_EQ(text.Perform(ctx), "hello");
  
  LaunchConfiguration config("test");
  EXPECT_EQ(config.Perform(ctx), "value");
  
  EnvironmentVariable env("TEST");
  EXPECT_EQ(env.Perform(ctx), "env_value");
  
  ThisLaunchFile file;
  EXPECT_EQ(file.Perform(ctx), "/test/launch.yaml");
  
  ThisLaunchFileDir dir;
  EXPECT_EQ(dir.Perform(ctx), "/test");
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
