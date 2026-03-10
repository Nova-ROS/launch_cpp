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
 * @file test_dependency_manager.cpp
 * @brief Unit tests for DependencyManager and DependencyResolver
 */

#include <gtest/gtest.h>
#include <memory>
#include "launch_cpp/dependency_manager.hpp"
#include "launch_cpp/actions/execute_process.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/safety/osal.hpp"
#include "launch_cpp/event.hpp"
#include "launch_cpp/event_handler.hpp"
#include "launch_cpp/error_code.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"

using namespace launch_cpp;

// Mock LaunchContext for testing
class MockLaunchContext : public LaunchContext
{
 public:
  void RegisterEventHandler(const EventHandlerPtr&) override {}
  void UnregisterEventHandler(const EventHandler*) override {}
  const EventHandlerVector& GetEventHandlers() const override { return handlers_; }
  void SetLaunchConfiguration(const std::string&, const std::string&) override {}
  Result<std::string> GetLaunchConfiguration(const std::string&) const override {
    return Result<std::string>("");
  }
  bool HasLaunchConfiguration(const std::string&) const override { return false; }
  std::string GetEnvironmentVariable(const std::string&) const override { return ""; }
  void SetEnvironmentVariable(const std::string&, const std::string&) override {}
  void EmitEvent(EventPtr) override {}
  void SetCurrentLaunchFile(const std::string&) override {}
  std::string GetCurrentLaunchFile() const override { return ""; }

 private:
  EventHandlerVector handlers_;
};

// Helper to create text substitution
SubstitutionPtr text(const std::string& str)
{
  return std::make_shared<TextSubstitution>(str);
}

// ============================================================================
// DependencyResolver Tests
// ============================================================================

TEST(DependencyResolverTest, ResolveLinearDependencies)
{
  std::vector<NodeConfig> nodes = {
    {"A", {}},
    {"B", {"A"}},
    {"C", {"B"}}
  };
  
  DependencyResolver resolver;
  auto result = resolver.Resolve(nodes);
  
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.order.size(), 3);
  EXPECT_EQ(result.order[0], "A");
  EXPECT_EQ(result.order[1], "B");
  EXPECT_EQ(result.order[2], "C");
}

TEST(DependencyResolverTest, ResolveDiamondDependencies)
{
  std::vector<NodeConfig> nodes = {
    {"A", {}},
    {"B", {"A"}},
    {"C", {"A"}},
    {"D", {"B", "C"}}
  };
  
  DependencyResolver resolver;
  auto result = resolver.Resolve(nodes);
  
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.order.size(), 4);
  EXPECT_EQ(result.order[0], "A");
  // B and C can be in any order, but both must come after A and before D
  EXPECT_TRUE((result.order[1] == "B" && result.order[2] == "C") ||
              (result.order[1] == "C" && result.order[2] == "B"));
  EXPECT_EQ(result.order[3], "D");
}

TEST(DependencyResolverTest, DetectCircularDependency)
{
  std::vector<NodeConfig> nodes = {
    {"A", {"C"}},
    {"B", {"A"}},
    {"C", {"B"}}
  };
  
  DependencyResolver resolver;
  auto result = resolver.Resolve(nodes);
  
  EXPECT_FALSE(result.success);
  EXPECT_TRUE(resolver.HasCircularDependency(nodes));
}

TEST(DependencyResolverTest, DetectMissingDependency)
{
  std::vector<NodeConfig> nodes = {
    {"A", {}},
    {"B", {"C"}}  // C doesn't exist
  };
  
  DependencyResolver resolver;
  auto result = resolver.Resolve(nodes);
  
  EXPECT_FALSE(result.success);
  EXPECT_NE(result.error_message.find("Missing"), std::string::npos);
}

// ============================================================================
// DependencyManager Tests
// ============================================================================

TEST(DependencyManagerTest, AddProcess)
{
  DependencyManager manager;
  
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("test")};
  auto action = std::make_shared<ExecuteProcess>(options);
  
  auto error = manager.AddProcess("test_process", action, {});
  EXPECT_FALSE(error.IsError());
  EXPECT_EQ(manager.GetProcessCount(), 1);
}

TEST(DependencyManagerTest, AddDuplicateProcess)
{
  DependencyManager manager;
  
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("test")};
  auto action = std::make_shared<ExecuteProcess>(options);
  
  manager.AddProcess("test", action, {});
  auto error = manager.AddProcess("test", action, {});
  
  EXPECT_TRUE(error.IsError());
}

TEST(DependencyManagerTest, GetProcess)
{
  DependencyManager manager;
  
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("test")};
  auto action = std::make_shared<ExecuteProcess>(options);
  
  manager.AddProcess("my_process", action, {});
  
  auto retrieved = manager.GetProcess("my_process");
  EXPECT_NE(retrieved, nullptr);
  
  auto not_found = manager.GetProcess("nonexistent");
  EXPECT_EQ(not_found, nullptr);
}

TEST(DependencyManagerTest, IsReady)
{
  DependencyManager manager;
  
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("test")};
  
  auto action_a = std::make_shared<ExecuteProcess>(options);
  auto action_b = std::make_shared<ExecuteProcess>(options);
  
  manager.AddProcess("A", action_a, {});
  manager.AddProcess("B", action_b, {"A"});
  
  std::set<std::string> completed;
  
  // A has no dependencies, should be ready
  EXPECT_TRUE(manager.IsReady("A", completed));
  
  // B depends on A, should not be ready yet
  EXPECT_FALSE(manager.IsReady("B", completed));
  
  // Mark A as completed
  completed.insert("A");
  
  // Now B should be ready
  EXPECT_TRUE(manager.IsReady("B", completed));
}

TEST(DependencyManagerTest, ResolveDependencies)
{
  DependencyManager manager;
  
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("test")};
  
  auto action_a = std::make_shared<ExecuteProcess>(options);
  auto action_b = std::make_shared<ExecuteProcess>(options);
  auto action_c = std::make_shared<ExecuteProcess>(options);
  
  manager.AddProcess("A", action_a, {});
  manager.AddProcess("B", action_b, {"A"});
  manager.AddProcess("C", action_c, {"B"});
  
  auto result = manager.ResolveDependencies();
  
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.order.size(), 3);
  EXPECT_EQ(result.order[0], "A");
  EXPECT_EQ(result.order[1], "B");
  EXPECT_EQ(result.order[2], "C");
}

TEST(DependencyManagerTest, ResolveWithCircularDependency)
{
  DependencyManager manager;
  
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("test")};
  
  auto action_a = std::make_shared<ExecuteProcess>(options);
  auto action_b = std::make_shared<ExecuteProcess>(options);
  auto action_c = std::make_shared<ExecuteProcess>(options);
  
  manager.AddProcess("A", action_a, {"C"});
  manager.AddProcess("B", action_b, {"A"});
  manager.AddProcess("C", action_c, {"B"});
  
  auto result = manager.ResolveDependencies();
  
  EXPECT_FALSE(result.success);
}

TEST(DependencyManagerTest, Clear)
{
  DependencyManager manager;
  
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("test")};
  auto action = std::make_shared<ExecuteProcess>(options);
  
  manager.AddProcess("test", action, {});
  EXPECT_EQ(manager.GetProcessCount(), 1);
  
  manager.Clear();
  EXPECT_EQ(manager.GetProcessCount(), 0);
  EXPECT_EQ(manager.GetProcess("test"), nullptr);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
