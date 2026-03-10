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


// Test: Substitutions
#include <gtest/gtest.h>
#include "launch_cpp/substitutions/text_substitution.hpp"

using namespace launch_cpp;

// Mock LaunchContext for testing
class MockLaunchContext : public LaunchContext
{
 public:
  void register_event_handler(const EventHandlerPtr&) override {}
  void unregister_event_handler(const EventHandler*) override {}
  const EventHandlerVector& get_event_handlers() const override { return handlers_; }
  void set_launch_configuration(const std::string&, const std::string&) override {}
  Result<std::string> get_launch_configuration(const std::string&) const override
  {
    return Result<std::string>("test_value");
  }
  bool has_launch_configuration(const std::string&) const override { return true; }
  std::string get_environment_variable(const std::string&) const override { return ""; }
  void set_environment_variable(const std::string&, const std::string&) override {}
  void emit_event(EventPtr) override {}
  
 private:
  EventHandlerVector handlers_;
};

TEST(SubstitutionTest, TextSubstitution)
{
  MockLaunchContext context;
  TextSubstitution sub("Hello World");
  
  std::string result = sub.perform(context);
  EXPECT_EQ(result, "Hello World");
}

TEST(SubstitutionTest, TextSubstitutionOperator)
{
  MockLaunchContext context;
  TextSubstitution sub("Test");
  
  std::string result = sub(context);
  EXPECT_EQ(result, "Test");
}
