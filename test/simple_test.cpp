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


// Simple test file to verify basic functionality
#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

// Include core headers
#include "launch_cpp/types.hpp"
#include "launch_cpp/error_code.hpp"
#include "launch_cpp/singleton.hpp"
#include "launch_cpp/thread_pool.hpp"
#include "launch_cpp/event.hpp"
#include "launch_cpp/launch_description_entity.hpp"
#include "launch_cpp/launch_description.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"
#include "launch_cpp/event_handler.hpp"
#include "launch_cpp/launch_context.hpp"

using namespace launch_cpp;

// Test counter
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
  std::cout << "Running " #name "... "; \
  try { \
    test_##name(); \
    std::cout << "PASSED" << std::endl; \
    tests_passed++; \
  } catch (const std::exception& e) { \
    std::cout << "FAILED: " << e.what() << std::endl; \
    tests_failed++; \
  } \
} while(0)

#define ASSERT_TRUE(expr) do { \
  if (!(expr)) { \
    throw std::runtime_error("Assertion failed: " #expr); \
  } \
} while(0)

#define ASSERT_FALSE(expr) do { \
  if (expr) { \
    throw std::runtime_error("Assertion failed: NOT(" #expr ")"); \
  } \
} while(0)

#define ASSERT_EQ(a, b) do { \
  if ((a) != (b)) { \
    throw std::runtime_error("Assertion failed: " #a " == " #b); \
  } \
} while(0)

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

TEST(error_code_success)
{
  Error err;
  ASSERT_TRUE(err.IsSuccess());
  ASSERT_FALSE(err.IsError());
}

TEST(error_code_error)
{
  Error err(ErrorCode::kInvalidArgument);
  ASSERT_TRUE(err.IsError());
  ASSERT_FALSE(err.IsSuccess());
  ASSERT_EQ(err.GetCode(), ErrorCode::kInvalidArgument);
}

TEST(result_success)
{
  Result<int> result(42);
  ASSERT_TRUE(result.HasValue());
  ASSERT_FALSE(result.HasError());
  ASSERT_EQ(result.GetValue(), 42);
}

TEST(result_error)
{
  Result<int> result(Error(ErrorCode::kInvalidArgument));
  ASSERT_TRUE(result.HasError());
  ASSERT_FALSE(result.HasValue());
  ASSERT_EQ(result.GetError().GetCode(), ErrorCode::kInvalidArgument);
}

TEST(thread_pool_creation)
{
  ThreadPool pool(4);
  ASSERT_EQ(pool.GetThreadCount(), 4U);
}

TEST(thread_pool_submit)
{
  ThreadPool pool(2);
  std::atomic<int> counter(0);
  
  for (int i = 0; i < 10; ++i) {
    Error err = pool.Submit([&counter]() { counter++; });
    ASSERT_TRUE(err.IsSuccess());
  }
  
  pool.Shutdown();
  ASSERT_EQ(counter.load(), 10);
}

TEST(text_substitution)
{
  MockLaunchContext ctx;
  TextSubstitution sub("Hello World");
  
  std::string result = sub.Perform(ctx);
  ASSERT_EQ(result, "Hello World");
}

TEST(launch_description_creation)
{
  LaunchDescription desc;
  ASSERT_EQ(desc.GetEntities().size(), 0U);
}

TEST(launch_description_add)
{
  LaunchDescription desc;
  
  // Create a simple entity
  class SimpleEntity : public LaunchDescriptionEntity
  {
   public:
    Result<LaunchDescriptionEntityVector> Visit(LaunchContext&) override
    {
      return Result<LaunchDescriptionEntityVector>(LaunchDescriptionEntityVector{});
    }
  };
  
  desc.Add(std::make_shared<SimpleEntity>());
  ASSERT_EQ(desc.GetEntities().size(), 1U);
}

TEST(singleton_pattern)
{
  // Test singleton with a simple test class
  class TestClass
  {
   public:
    int value = 0;
  };
  
  ASSERT_FALSE(Singleton<TestClass>::IsInitialized());
  
  TestClass& instance = Singleton<TestClass>::Instance();
  instance.value = 42;
  
  ASSERT_TRUE(Singleton<TestClass>::IsInitialized());
  
  TestClass& instance2 = Singleton<TestClass>::Instance();
  ASSERT_EQ(instance2.value, 42);
}

int main()
{
  std::cout << "=== launch_cpp Simple Tests ===" << std::endl;
  std::cout << std::endl;
  
  RUN_TEST(error_code_success);
  RUN_TEST(error_code_error);
  RUN_TEST(result_success);
  RUN_TEST(result_error);
  RUN_TEST(thread_pool_creation);
  RUN_TEST(thread_pool_submit);
  RUN_TEST(text_substitution);
  RUN_TEST(launch_description_creation);
  RUN_TEST(launch_description_add);
  RUN_TEST(singleton_pattern);
  
  std::cout << std::endl;
  std::cout << "=== Test Summary ===" << std::endl;
  std::cout << "Passed: " << tests_passed << std::endl;
  std::cout << "Failed: " << tests_failed << std::endl;
  
  return tests_failed == 0 ? 0 : 1;
}
