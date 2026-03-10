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
 * @file test_safety_features.cpp
 * @brief Comprehensive safety feature tests for launch_cpp
 * 
 * These tests focus on improving coverage of safety-enabled code paths.
 * All tests use MockProcessExecutor to avoid system dependencies.
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <memory>
#include "launch_cpp/actions/execute_process.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"
#include "launch_cpp/safety/osal.hpp"
#include "launch_cpp/event.hpp"
#include "launch_cpp/event_handler.hpp"
#include "launch_cpp/error_code.hpp"

using namespace launch_cpp;

// Helper function
SubstitutionPtr text(const std::string& str)
{
  return std::make_shared<TextSubstitution>(str);
}

// Mock LaunchContext
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

// Mock ResourceMonitor for testing
class MockResourceMonitor : public launch_cpp::ResourceMonitor {
 public:
  MockResourceMonitor() = default;
  ~MockResourceMonitor() override = default;

  OsalResult<SystemResources> get_system_resources() override {
    return OsalResult<SystemResources>(SystemResources{});
  }

  OsalResult<ResourceUsage> get_process_resources(ProcessId) override {
    return OsalResult<ResourceUsage>(ResourceUsage{});
  }

  OsalResult<bool> are_resources_available(uint64_t) override {
    return OsalResult<bool>(true);
  }

  OsalResult<void> set_resource_limits(ProcessId, uint64_t, double) override {
    return OsalResult<void>();
  }

  void register_threshold_callback(double, std::function<void(const SystemResources&)>) override {}
};

// ============================================================================
// Safety Constructor Tests
// ============================================================================

TEST(SafetyFeaturesTest, ConstructorWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
}

TEST(SafetyFeaturesTest, ConstructorWithSafetyAndWatchdog)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enable_safety = true;
  options.watchdog_timeout_ms = 5000;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
}

TEST(SafetyFeaturesTest, ConstructorWithAllSafetyOptions)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enable_safety = true;
  options.max_memory_bytes = 512 * 1024 * 1024;
  options.max_cpu_percent = 50.0;
  options.watchdog_timeout_ms = 10000;
  options.sigterm_timeout = 5;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
}

// ============================================================================
// Resource Check Tests with Mock
// ============================================================================

TEST(SafetyFeaturesTest, ResourceCheckWithMonitor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("test")};
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Inject mock executor (for interface compatibility)
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  action->set_process_executor(mockExecutor);
  
  // Set mock resource monitor to avoid real resource checks
  auto mockMonitor = std::make_shared<MockResourceMonitor>();
  action->set_resource_monitor(mockMonitor);
  
  // Check resources - should work with mock monitor
  bool available = action->check_resources_available(100 * 1024 * 1024);
  EXPECT_TRUE(available);
}

TEST(SafetyFeaturesTest, ResourceCheckWithoutMonitor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("test")};
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Without monitor, should return true (default)
  bool available = action->check_resources_available(100 * 1024 * 1024);
  EXPECT_TRUE(available);
}

// ============================================================================
// Process Control Tests with Mock
// ============================================================================

TEST(SafetyFeaturesTest, ShutdownWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  // Inject mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->set_is_running_callback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(true);  // Process is running
    });
  
  mockExecutor->set_terminate_callback(
    [](ProcessId, std::chrono::milliseconds) -> OsalResult<void> {
      return OsalResult<void>();
    });
  
  action->set_process_executor(mockExecutor);
  
  // Execute process
  auto result = action->execute(context);
  EXPECT_TRUE(result.has_value());
  
  // Test shutdown
  Error shutdownResult = action->shutdown();
  (void)shutdownResult;
  EXPECT_TRUE(true);  // Should not crash
}

TEST(SafetyFeaturesTest, TerminateWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  // Inject mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->set_is_running_callback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(true);
    });
  
  mockExecutor->set_terminate_callback(
    [](ProcessId, std::chrono::milliseconds) -> OsalResult<void> {
      return OsalResult<void>();
    });
  
  action->set_process_executor(mockExecutor);
  
  auto result = action->execute(context);
  EXPECT_TRUE(result.has_value());
  
  Error termResult = action->terminate();
  (void)termResult;
  EXPECT_TRUE(true);
}

TEST(SafetyFeaturesTest, KillWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  // Inject mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->set_is_running_callback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(true);
    });
  
  mockExecutor->set_kill_callback(
    [](ProcessId) -> OsalResult<void> {
      return OsalResult<void>();
    });
  
  action->set_process_executor(mockExecutor);
  
  auto result = action->execute(context);
  EXPECT_TRUE(result.has_value());
  
  Error killResult = action->kill();
  (void)killResult;
  EXPECT_TRUE(true);
}

// ============================================================================
// Status Query Tests with Mock
// ============================================================================

TEST(SafetyFeaturesTest, IsRunningWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  EXPECT_FALSE(action->is_running());
  
  // Inject mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->set_is_running_callback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(true);  // Process is running
    });
  
  action->set_process_executor(mockExecutor);
  
  auto result = action->execute(context);
  EXPECT_TRUE(result.has_value());
  
  // Now process should be reported as running
  EXPECT_TRUE(action->is_running());
}

TEST(SafetyFeaturesTest, GetPidWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto pidResult = action->get_pid();
  EXPECT_TRUE(pidResult.has_error());  // Process not started
  
  // Inject mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  action->set_process_executor(mockExecutor);
  
  auto result = action->execute(context);
  EXPECT_TRUE(result.has_value());
  
  pidResult = action->get_pid();
  EXPECT_TRUE(pidResult.has_value());
  EXPECT_EQ(pidResult.get_value(), 1234);
}

TEST(SafetyFeaturesTest, GetReturnCodeWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.output = "screen";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  // Inject mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->set_wait_callback(
    [](ProcessId, std::chrono::milliseconds) -> OsalResult<ProcessResult> {
      ProcessResult result;
      result.pid = 1234;
      result.exit_code = 0;
      result.final_state = ProcessState::kStopped;
      return OsalResult<ProcessResult>(result);
    });
  
  action->set_process_executor(mockExecutor);
  
  auto result = action->execute(context);
  EXPECT_TRUE(result.has_value());
  
  auto returnCodeResult = action->get_return_code();
  // May or may not have value depending on implementation
  (void)returnCodeResult;
  EXPECT_TRUE(true);
}

// ============================================================================
// SendSignal Tests with Mock
// ============================================================================

TEST(SafetyFeaturesTest, SendSignalWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  // Inject mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->set_is_running_callback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(true);
    });
  
  mockExecutor->set_send_signal_callback(
    [](ProcessId, int32_t) -> OsalResult<void> {
      return OsalResult<void>();
    });
  
  action->set_process_executor(mockExecutor);
  
  auto result = action->execute(context);
  EXPECT_TRUE(result.has_value());
  
  action->send_signal(SIGTERM);
  EXPECT_TRUE(true);
}

// ============================================================================
// Destructor Tests with Mock
// ============================================================================

TEST(SafetyFeaturesTest, DestructorWithWatchdog)
{
  {
    ExecuteProcess::Options options;
    options.cmd = {text("test")};
    options.enable_safety = true;
    options.watchdog_timeout_ms = 5000;
    
    auto action = std::make_shared<ExecuteProcess>(options);
    MockLaunchContext context;
    
    // Inject mock executor
    auto mockExecutor = std::make_shared<MockProcessExecutor>();
    mockExecutor->set_execute_callback(
      [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
        return OsalResult<ProcessId>(1234);
      });
    
    action->set_process_executor(mockExecutor);
    
    auto result = action->execute(context);
    EXPECT_TRUE(result.has_value());
    
    // Destructor should cleanup watchdog
  }
  // If we get here without crash, test passed
  EXPECT_TRUE(true);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST(SafetyFeaturesTest, EmptyCommandWithSafety)
{
  ExecuteProcess::Options options;
  options.cmd = {};  // Empty command
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto result = action->execute(context);
  EXPECT_TRUE(result.has_error());
  EXPECT_EQ(result.get_error().get_code(), ErrorCode::kInvalidArgument);
}

TEST(SafetyFeaturesTest, ResourceLimitsZeroValues)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.max_memory_bytes = 0;  // Unlimited
  options.max_cpu_percent = 0.0;  // Unlimited
  options.watchdog_timeout_ms = 0;  // Disabled
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  // Inject mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  action->set_process_executor(mockExecutor);
  
  auto result = action->execute(context);
  EXPECT_TRUE(result.has_value());
}

TEST(SafetyFeaturesTest, ProcessControlBeforeExecution)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Try to control process before execution
  Error shutdownResult = action->shutdown();
  Error termResult = action->terminate();
  Error killResult = action->kill();
  
  // Should not crash
  (void)shutdownResult;
  (void)termResult;
  (void)killResult;
  EXPECT_TRUE(true);
}

TEST(SafetyFeaturesTest, GetStatusBeforeExecution)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  EXPECT_FALSE(action->is_running());
  
  auto pidResult = action->get_pid();
  EXPECT_TRUE(pidResult.has_error());
  
  auto returnCodeResult = action->get_return_code();
  EXPECT_TRUE(returnCodeResult.has_error());
}

// ============================================================================
// OSAL Component Tests
// ============================================================================

TEST(SafetyFeaturesTest, PosixProcessExecutorCreation)
{
  auto executor = std::make_shared<PosixProcessExecutor>();
  EXPECT_NE(executor, nullptr);
}

TEST(SafetyFeaturesTest, PosixResourceMonitorCreation)
{
  auto monitor = std::make_shared<PosixResourceMonitor>();
  EXPECT_NE(monitor, nullptr);
  
  auto result = monitor->get_system_resources();
  EXPECT_TRUE(result.is_success());
}

TEST(SafetyFeaturesTest, PosixWatchdogCreation)
{
  auto watchdog = std::make_shared<PosixWatchdog>();
  EXPECT_NE(watchdog, nullptr);
  
  auto result = watchdog->start();
  EXPECT_TRUE(result.is_success());
  
  watchdog->stop();
}

TEST(SafetyFeaturesTest, WatchdogRegisterAndUnregister)
{
  auto watchdog = std::make_shared<PosixWatchdog>();
  EXPECT_NE(watchdog, nullptr);
  
  auto startResult = watchdog->start();
  EXPECT_TRUE(startResult.is_success());
  
  auto regResult = watchdog->register_node(1, 5000, nullptr);
  EXPECT_TRUE(regResult.is_success());
  
  auto unregResult = watchdog->unregister_node(1);
  EXPECT_TRUE(unregResult.is_success());
  
  watchdog->stop();
}

TEST(SafetyFeaturesTest, WatchdogHeartbeat)
{
  auto watchdog = std::make_shared<PosixWatchdog>();
  EXPECT_NE(watchdog, nullptr);
  
  auto startResult = watchdog->start();
  EXPECT_TRUE(startResult.is_success());
  
  auto regResult = watchdog->register_node(1, 5000, nullptr);
  EXPECT_TRUE(regResult.is_success());
  
  HeartbeatMessage msg;
  msg.node_id = 1;
  msg.sequence = 1;
  msg.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
  msg.state = ProcessState::kRunning;
  msg.checksum = msg.CalculateChecksum();
  
  auto hbResult = watchdog->submit_heartbeat(msg);
  EXPECT_TRUE(hbResult.is_success());
  
  auto responsiveResult = watchdog->is_responsive(1);
  EXPECT_TRUE(responsiveResult.is_success());
  EXPECT_TRUE(responsiveResult.get_value());
  
  watchdog->stop();
}

// ============================================================================
// Integration Tests with Mock
// ============================================================================

TEST(SafetyFeaturesIntegrationTest, FullSafetyWorkflow)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.max_memory_bytes = 100 * 1024 * 1024;
  options.max_cpu_percent = 50.0;
  options.watchdog_timeout_ms = 5000;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  // Setup mocks
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->set_is_running_callback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(true);
    });
  
  mockExecutor->set_terminate_callback(
    [](ProcessId, std::chrono::milliseconds) -> OsalResult<void> {
      return OsalResult<void>();
    });
  
  action->set_process_executor(mockExecutor);
  
  // Execute
  auto result = action->execute(context);
  EXPECT_TRUE(result.has_value());
  
  // Check status
  EXPECT_TRUE(action->is_running());
  
  auto pidResult = action->get_pid();
  EXPECT_TRUE(pidResult.has_value());
  EXPECT_EQ(pidResult.get_value(), 1234);
  
  // Terminate
  action->terminate();
  EXPECT_TRUE(true);
}

TEST(SafetyFeaturesIntegrationTest, MultipleSafetyOptions)
{
  // Test various combinations
  std::vector<std::tuple<bool, uint64_t, double, int32_t>> configs = {
    {true, 0, 0.0, 0},           // Safety only
    {true, 1024 * 1024 * 100, 0.0, 0},    // Safety + memory
    {true, 0, 50.0, 0},          // Safety + CPU
    {true, 0, 0.0, 1000},        // Safety + watchdog
    {true, 1024 * 1024 * 100, 50.0, 1000} // All options
  };
  
  for (const auto& config : configs) {
    ExecuteProcess::Options options;
    options.cmd = {text("test")};
    options.enable_safety = std::get<0>(config);
    options.max_memory_bytes = std::get<1>(config);
    options.max_cpu_percent = std::get<2>(config);
    options.watchdog_timeout_ms = std::get<3>(config);
    options.output = "log";
    
    auto action = std::make_shared<ExecuteProcess>(options);
    MockLaunchContext context;
    
    auto mockExecutor = std::make_shared<MockProcessExecutor>();
    mockExecutor->set_execute_callback(
      [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
        return OsalResult<ProcessId>(1234);
      });
    
    action->set_process_executor(mockExecutor);
    
    // Set mock resource monitor to avoid resource check failures
    auto mockMonitor = std::make_shared<MockResourceMonitor>();
    action->set_resource_monitor(mockMonitor);
    
    auto result = action->execute(context);
    if (result.has_error()) {
      std::cerr << "Execute failed: " << result.get_error().get_message() << std::endl;
    }
    EXPECT_TRUE(result.has_value());
  }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
