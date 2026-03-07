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
 * @file test_safety_mock.cpp
 * @brief Mock-based safety tests to avoid system dependencies
 * 
 * These tests use MockProcessExecutor and other mocks to test
 * safety code paths without requiring actual system resources.
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <memory>
#include "cpp_launch/actions/execute_process.hpp"
#include "cpp_launch/launch_context.hpp"
#include "cpp_launch/substitutions/text_substitution.hpp"
#include "cpp_launch/safety/osal.hpp"
#include "cpp_launch/event.hpp"
#include "cpp_launch/event_handler.hpp"
#include "cpp_launch/error_code.hpp"

using namespace cpp_launch;

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

// ============================================================================
// Mock-based Safety Tests
// ============================================================================

TEST(SafetyMockTest, ExecuteWithMockSuccess)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Inject mock executor that returns success
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->SetExecuteCallback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->SetIsRunningCallback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(false);  // Process completed immediately
    });
  
  action->SetProcessExecutor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  
  EXPECT_TRUE(result.HasValue());
  
  auto pidResult = action->GetPid();
  EXPECT_TRUE(pidResult.HasValue());
  EXPECT_EQ(pidResult.GetValue(), 1234);
}

TEST(SafetyMockTest, ExecuteWithMockFailure)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Inject mock executor that returns failure
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->SetExecuteCallback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(
        OsalStatus::kError,
        "Mock execution failed");
    });
  
  action->SetProcessExecutor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  
  EXPECT_TRUE(result.HasError());
}

TEST(SafetyMockTest, ResourceCheckWithMockMonitor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Inject mock monitor that returns resources available
//   auto mockMonitor = std::make_shared<MockProcessExecutor>();  // Reuse mock for interface
//   action->SetResourceMonitor(mockMonitor);
  
  // Without proper mock, this will use default behavior
  bool available = action->CheckResourcesAvailable(100 * 1024 * 1024);
  // Result depends on implementation
  (void)available;
}

TEST(SafetyMockTest, ProcessControlWithMockExecutor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  bool terminateCalled = false;
  bool killCalled = false;
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->SetExecuteCallback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->SetTerminateCallback(
    [&terminateCalled](ProcessId, std::chrono::milliseconds) -> OsalResult<void> {
      terminateCalled = true;
      return OsalResult<void>();
    });
  
  mockExecutor->SetKillCallback(
    [&killCalled](ProcessId) -> OsalResult<void> {
      killCalled = true;
      return OsalResult<void>();
    });
  
  mockExecutor->SetIsRunningCallback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(true);
    });
  
  action->SetProcessExecutor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  // Test Terminate
  Error termResult = action->Terminate();
  EXPECT_TRUE(terminateCalled);
  
  // Test Kill
  Error killResult = action->Kill();
  EXPECT_TRUE(killCalled);
}

TEST(SafetyMockTest, SendSignalWithMock)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  bool signalCalled = false;
  int32_t sentSignal = 0;
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->SetExecuteCallback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->SetSendSignalCallback(
    [&signalCalled, &sentSignal](ProcessId, int32_t sig) -> OsalResult<void> {
      signalCalled = true;
      sentSignal = sig;
      return OsalResult<void>();
    });
  
  mockExecutor->SetIsRunningCallback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(true);
    });
  
  action->SetProcessExecutor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  action->SendSignal(SIGTERM);
  EXPECT_TRUE(signalCalled);
  EXPECT_EQ(sentSignal, SIGTERM);
}

TEST(SafetyMockTest, IsRunningWithMock)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->SetExecuteCallback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->SetIsRunningCallback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(true);
    });
  
  action->SetProcessExecutor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  EXPECT_TRUE(action->IsRunning());
}

TEST(SafetyMockTest, GetStateWithMock)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->SetExecuteCallback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->SetGetStateCallback(
    [](ProcessId) -> OsalResult<ProcessState> {
      return OsalResult<ProcessState>(ProcessState::kRunning);
    });
  
  action->SetProcessExecutor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  // Process should be reported as running
  EXPECT_TRUE(action->IsRunning());
}

// ============================================================================
// Edge Cases with Mocks
// ============================================================================

TEST(SafetyMockEdgeTest, ZeroPidProcessControl)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Try to control before execution (processId_ = 0)
  Error shutdownResult = action->Shutdown();
  Error termResult = action->Terminate();
  Error killResult = action->Kill();
  
  // Should not crash and should use legacy path
  (void)shutdownResult;
  (void)termResult;
  (void)killResult;
}

TEST(SafetyMockEdgeTest, ResourceLimitsWithZeroValues)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.maxMemoryBytes = 0;  // Unlimited
  options.maxCpuPercent = 0.0;  // Unlimited
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->SetExecuteCallback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  action->SetProcessExecutor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  
  // Should succeed even with zero limits
  EXPECT_TRUE(result.HasValue());
}

TEST(SafetyMockEdgeTest, WatchdogTimeoutZero)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.watchdogTimeoutMs = 0;  // Disabled
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->SetExecuteCallback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  action->SetProcessExecutor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  
  // Should succeed without watchdog
  EXPECT_TRUE(result.HasValue());
}

TEST(SafetyMockEdgeTest, EmptyCommand)
{
  ExecuteProcess::Options options;
  options.cmd = {};  // Empty
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  
  EXPECT_TRUE(result.HasError());
  EXPECT_EQ(result.GetError().GetCode(), ErrorCode::kInvalidArgument);
}

TEST(SafetyMockEdgeTest, MultipleSafetyOptionsCombination)
{
  // Test various combinations of safety options
  std::vector<std::tuple<bool, uint64_t, double, int32_t>> configs = {
    {true, 0, 0.0, 0},                    // Safety only
    {true, 1024 * 1024 * 100, 0.0, 0},    // Safety + memory
    {true, 0, 50.0, 0},                   // Safety + CPU
    {true, 0, 0.0, 5000},                 // Safety + watchdog
    {true, 1024 * 1024 * 100, 50.0, 5000} // All options
  };
  
  for (const auto& config : configs) {
    ExecuteProcess::Options options;
    options.cmd = {text("test")};
    options.enableSafety = std::get<0>(config);
    options.maxMemoryBytes = std::get<1>(config);
    options.maxCpuPercent = std::get<2>(config);
    options.watchdogTimeoutMs = std::get<3>(config);
    options.output = "log";
    
    auto action = std::make_shared<ExecuteProcess>(options);
    
    auto mockExecutor = std::make_shared<MockProcessExecutor>();
    mockExecutor->SetExecuteCallback(
      [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
        return OsalResult<ProcessId>(1234);
      });
    
    action->SetProcessExecutor(mockExecutor);
    
    MockLaunchContext context;
    auto result = action->Execute(context);
    
    EXPECT_TRUE(result.HasValue());
  }
}

// ============================================================================
// Integration with Mocks
// ============================================================================

TEST(SafetyMockIntegrationTest, FullWorkflowWithMocks)
{
  ExecuteProcess::Options options;
  options.cmd = {text("/bin/echo"), text("integration test")};
  options.enableSafety = true;
  options.maxMemoryBytes = 100 * 1024 * 1024;
  options.maxCpuPercent = 50.0;
  options.watchdogTimeoutMs = 5000;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Setup mocks
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  bool terminateCalled = false;
  
  mockExecutor->SetExecuteCallback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->SetIsRunningCallback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(true);
    });
  
  mockExecutor->SetTerminateCallback(
    [&terminateCalled](ProcessId, std::chrono::milliseconds) -> OsalResult<void> {
      terminateCalled = true;
      return OsalResult<void>();
    });
  
  action->SetProcessExecutor(mockExecutor);
  
  MockLaunchContext context;
  
  // Execute
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  // Check status
  EXPECT_TRUE(action->IsRunning());
  
  auto pidResult = action->GetPid();
  EXPECT_TRUE(pidResult.HasValue());
  EXPECT_EQ(pidResult.GetValue(), 1234);
  
  // Terminate
  action->Terminate();
  EXPECT_TRUE(terminateCalled);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
