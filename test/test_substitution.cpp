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

// Test: Substitutions
#include <gtest/gtest.h>
#include "cpp_launch/substitutions/text_substitution.hpp"

using namespace cpp_launch;

// Mock LaunchContext for testing
class MockLaunchContext : public LaunchContext
{
 public:
  void RegisterEventHandler(const EventHandlerPtr&) override {}
  void UnregisterEventHandler(const EventHandler*) override {}
  const EventHandlerVector& GetEventHandlers() const override { return handlers_; }
  void SetLaunchConfiguration(const std::string&, const std::string&) override {}
  Result<std::string> GetLaunchConfiguration(const std::string&) const override
  {
    return Result<std::string>("test_value");
  }
  bool HasLaunchConfiguration(const std::string&) const override { return true; }
  std::string GetEnvironmentVariable(const std::string&) const override { return ""; }
  void SetEnvironmentVariable(const std::string&, const std::string&) override {}
  void EmitEvent(EventPtr) override {}
  
 private:
  EventHandlerVector handlers_;
};

TEST(SubstitutionTest, TextSubstitution)
{
  MockLaunchContext context;
  TextSubstitution sub("Hello World");
  
  std::string result = sub.Perform(context);
  EXPECT_EQ(result, "Hello World");
}

TEST(SubstitutionTest, TextSubstitutionOperator)
{
  MockLaunchContext context;
  TextSubstitution sub("Test");
  
  std::string result = sub(context);
  EXPECT_EQ(result, "Test");
}
