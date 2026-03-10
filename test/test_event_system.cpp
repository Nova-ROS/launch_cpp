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


// Comprehensive Event System Tests
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "launch_cpp/event.hpp"
#include "launch_cpp/event_queue.hpp"
#include "launch_cpp/event_dispatcher.hpp"
#include "launch_cpp/event_handler.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/launch_description_entity.hpp"

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
  void SetCurrentLaunchFile(const std::string& path) override { currentLaunchFile_ = path; }
  std::string GetCurrentLaunchFile() const override { return currentLaunchFile_; }

 private:
  EventHandlerVector handlers_;
  std::unordered_map<std::string, std::string> configs_;
  std::string currentLaunchFile_;
};

// Test: Event base class functionality
TEST(EventTest, BaseClass)
{
  ProcessStartedEvent event(1234, "test_process");
  
  EXPECT_EQ(event.get_pid(), 1234);
  EXPECT_EQ(event.get_name(), "test_process");
  EXPECT_STREQ(event.get_type(), "process_started");
  EXPECT_NE(event.get_timestamp().time_since_epoch().count(), 0);
}

// Test: ProcessStartedEvent
TEST(EventTest, ProcessStarted)
{
  ProcessStartedEvent event(1000, "/bin/echo hello");
  
  EXPECT_STREQ(event.get_type(), "process_started");
  EXPECT_EQ(event.get_pid(), 1000);
  EXPECT_EQ(event.get_name(), "/bin/echo hello");
}

// Test: ProcessExitedEvent
TEST(EventTest, ProcessExited)
{
  ProcessExitedEvent event(1000, 0, "/bin/echo hello");
  
  EXPECT_STREQ(event.get_type(), "process_exited");
  EXPECT_EQ(event.get_pid(), 1000);
  EXPECT_EQ(event.get_name(), "/bin/echo hello");
  EXPECT_EQ(event.get_return_code(), 0);
}

// Test: ProcessExitedEvent with error
TEST(EventTest, ProcessExitedWithError)
{
  ProcessExitedEvent event(1000, 1, "/bin/false");
  
  EXPECT_EQ(event.get_return_code(), 1);
}

// Test: ShutdownEvent
TEST(EventTest, ShutdownEvent)
{
  ShutdownEvent event("user_requested");
  
  EXPECT_STREQ(event.get_type(), "shutdown");
  EXPECT_EQ(event.get_reason(), "user_requested");
}

// Test: ShutdownEvent with error
TEST(EventTest, ShutdownEventWithError)
{
  ShutdownEvent event("error");
  
  EXPECT_EQ(event.get_reason(), "error");
}

// Test: EventQueue basic operations
TEST(EventQueueTest, BasicOperations)
{
  EventQueue queue;
  
  EXPECT_TRUE(queue.is_empty());
  EXPECT_EQ(queue.size(), 0U);
  
  // Push an event
  EventPtr event = std::make_shared<ProcessStartedEvent>(1234, "test");
  queue.push(event);
  
  EXPECT_FALSE(queue.is_empty());
  EXPECT_EQ(queue.size(), 1U);
  
  // Pop the event
  EventPtr popped;
  EXPECT_TRUE(queue.try_pop(popped));
  EXPECT_NE(popped, nullptr);
  EXPECT_TRUE(queue.is_empty());
}

// Test: EventQueue multiple events
TEST(EventQueueTest, MultipleEvents)
{
  EventQueue queue;
  
  // Push multiple events
  for (int i = 0; i < 10; ++i) {
    EventPtr event = std::make_shared<ProcessStartedEvent>(i, "test");
    queue.push(event);
  }
  
  EXPECT_EQ(queue.size(), 10U);
  
  // Pop all events
  int count = 0;
  EventPtr event;
  while (queue.try_pop(event)) {
    count++;
  }
  
  EXPECT_EQ(count, 10);
  EXPECT_TRUE(queue.is_empty());
}

// Test: EventQueue wait and pop
TEST(EventQueueTest, WaitAndPop)
{
  EventQueue queue;
  
  // Push from another thread
  std::thread producer([&queue]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EventPtr event = std::make_shared<ProcessStartedEvent>(1234, "test");
    queue.push(event);
  });
  
  // Wait for event
  EventPtr event;
  bool result = queue.wait_and_pop(event, std::chrono::milliseconds(200));
  
  EXPECT_TRUE(result);
  EXPECT_NE(event, nullptr);
  
  producer.join();
}

// Test: EventQueue wait timeout
TEST(EventQueueTest, WaitTimeout)
{
  EventQueue queue;
  
  // Wait without pushing anything
  EventPtr event;
  bool result = queue.wait_and_pop(event, std::chrono::milliseconds(50));
  
  EXPECT_FALSE(result);
  EXPECT_EQ(event, nullptr);
}

// Test: EventQueue clear
TEST(EventQueueTest, Clear)
{
  EventQueue queue;
  
  // Add events
  for (int i = 0; i < 5; ++i) {
    EventPtr event = std::make_shared<ProcessStartedEvent>(i, "test");
    queue.push(event);
  }
  
  EXPECT_EQ(queue.size(), 5U);
  
  // Clear
  queue.clear();
  
  EXPECT_TRUE(queue.is_empty());
  EXPECT_EQ(queue.size(), 0U);
}

// Test: EventQueue thread safety
TEST(EventQueueTest, ThreadSafety)
{
  EventQueue queue;
  std::atomic<int> push_count(0);
  std::atomic<int> pop_count(0);
  
  // Producer threads
  std::vector<std::thread> producers;
  for (int t = 0; t < 4; ++t) {
    producers.emplace_back([&queue, &push_count]() {
      for (int i = 0; i < 25; ++i) {
        EventPtr event = std::make_shared<ProcessStartedEvent>(i, "test");
        queue.push(event);
        push_count++;
      }
    });
  }
  
  // Consumer threads
  std::vector<std::thread> consumers;
  for (int t = 0; t < 2; ++t) {
    consumers.emplace_back([&queue, &pop_count]() {
      EventPtr event;
      while (pop_count < 100) {
        if (queue.try_pop(event)) {
          pop_count++;
        } else {
          std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
      }
    });
  }
  
  for (auto& t : producers) t.join();
  for (auto& t : consumers) t.join();
  
  EXPECT_EQ(push_count.load(), 100);
  EXPECT_EQ(pop_count.load(), 100);
}

// Test: EventDispatcher basic registration
TEST(EventDispatcherTest, BasicRegistration)
{
  EventDispatcher dispatcher;
  
  EXPECT_EQ(dispatcher.get_handler_count(), 0U);
  
  // Create a handler
  auto handler = std::make_shared<FunctionEventHandler>(
    [](const Event&) { return true; },
    [](const Event&, LaunchContext&) {
      return Result<LaunchDescriptionEntityVector>(LaunchDescriptionEntityVector{});
    }
  );
  
  dispatcher.register_handler(handler);
  EXPECT_EQ(dispatcher.get_handler_count(), 1U);
}

// Test: EventDispatcher unregister
TEST(EventDispatcherTest, Unregister)
{
  EventDispatcher dispatcher;
  
  auto handler = std::make_shared<FunctionEventHandler>(
    [](const Event&) { return true; },
    [](const Event&, LaunchContext&) {
      return Result<LaunchDescriptionEntityVector>(LaunchDescriptionEntityVector{});
    }
  );
  
  dispatcher.register_handler(handler);
  EXPECT_EQ(dispatcher.get_handler_count(), 1U);
  
  dispatcher.unregister_handler(handler.get());
  EXPECT_EQ(dispatcher.get_handler_count(), 0U);
}

// Test: EventDispatcher clear
TEST(EventDispatcherTest, Clear)
{
  EventDispatcher dispatcher;
  
  // Register multiple handlers
  for (int i = 0; i < 5; ++i) {
    auto handler = std::make_shared<FunctionEventHandler>(
      [](const Event&) { return true; },
      [](const Event&, LaunchContext&) {
        return Result<LaunchDescriptionEntityVector>(LaunchDescriptionEntityVector{});
      }
    );
    dispatcher.register_handler(handler);
  }
  
  EXPECT_EQ(dispatcher.get_handler_count(), 5U);
  
  dispatcher.clear_handlers();
  EXPECT_EQ(dispatcher.get_handler_count(), 0U);
}

// Test: EventDispatcher dispatch matching
TEST(EventDispatcherTest, DispatchMatching)
{
  EventDispatcher dispatcher;
  std::atomic<int> handler_called(0);
  
  // Create a handler that matches ProcessStartedEvent
  auto handler = std::make_shared<FunctionEventHandler>(
    [](const Event& e) {
      return std::string(e.get_type()) == "process_started";
    },
    [&handler_called](const Event&, LaunchContext&) {
      handler_called++;
      return Result<LaunchDescriptionEntityVector>(LaunchDescriptionEntityVector{});
    }
  );
  
  dispatcher.register_handler(handler);
  
  // Create a mock context
  MockLaunchContext ctx;
  
  // Dispatch a matching event
  ProcessStartedEvent event(1234, "test");
  auto result = dispatcher.dispatch(event, ctx);
  
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(handler_called.load(), 1);
}

// Test: EventDispatcher dispatch non-matching
TEST(EventDispatcherTest, DispatchNonMatching)
{
  EventDispatcher dispatcher;
  std::atomic<int> handler_called(0);
  
  // Create a handler that matches ProcessStartedEvent
  auto handler = std::make_shared<FunctionEventHandler>(
    [](const Event& e) {
      return std::string(e.get_type()) == "process_started";
    },
    [&handler_called](const Event&, LaunchContext&) {
      handler_called++;
      return Result<LaunchDescriptionEntityVector>(LaunchDescriptionEntityVector{});
    }
  );
  
  dispatcher.register_handler(handler);
  
  MockLaunchContext ctx;
  
  // Dispatch a non-matching event
  ShutdownEvent event("user_requested");
  auto result = dispatcher.dispatch(event, ctx);
  
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(handler_called.load(), 0);
}

// Test: EventDispatcher multiple handlers
TEST(EventDispatcherTest, MultipleHandlers)
{
  EventDispatcher dispatcher;
  std::atomic<int> handlers_called(0);
  
  // Register multiple handlers that all match
  for (int i = 0; i < 3; ++i) {
    auto handler = std::make_shared<FunctionEventHandler>(
      [](const Event&) { return true; },
      [&handlers_called](const Event&, LaunchContext&) {
        handlers_called++;
        return Result<LaunchDescriptionEntityVector>(LaunchDescriptionEntityVector{});
      }
    );
    dispatcher.register_handler(handler);
  }
  
  MockLaunchContext ctx;
  
  // Dispatch event
  ProcessStartedEvent event(1234, "test");
  auto result = dispatcher.dispatch(event, ctx);
  
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(handlers_called.load(), 3);
}

// Test: EventHandler base class
TEST(EventHandlerTest, BaseClass)
{
  FunctionEventHandler handler(
    [](const Event&) { return true; },
    [](const Event&, LaunchContext&) {
      return Result<LaunchDescriptionEntityVector>(LaunchDescriptionEntityVector{});
    }
  );
  
  ProcessStartedEvent event(1234, "test");
  EXPECT_TRUE(handler.matches(event));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
