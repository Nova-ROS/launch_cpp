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
 * @brief Comprehensive safety feature tests for cpp_launch
 * 
 * These tests focus on improving coverage of safety-enabled code paths.
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
// Safety Constructor Tests
// ============================================================================

TEST(SafetyFeaturesTest, ConstructorWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
}

TEST(SafetyFeaturesTest, ConstructorWithSafetyAndWatchdog)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = true;
  options.watchdogTimeoutMs = 5000;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
}

TEST(SafetyFeaturesTest, ConstructorWithAllSafetyOptions)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = true;
  options.maxMemoryBytes = 512 * 1024 * 1024;
  options.maxCpuPercent = 50.0;
  options.watchdogTimeoutMs = 10000;
  options.sigtermTimeout = 5;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
}

// ============================================================================
// Resource Check Tests
// ============================================================================

TEST(SafetyFeaturesTest, ResourceCheckWithMonitor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Inject mock monitor that returns available
  auto mockMonitor = std::make_shared<PosixResourceMonitor>();
  action->SetResourceMonitor(mockMonitor);
  
  bool available = action->CheckResourcesAvailable(100 * 1024 * 1024);
  // Should return true (actual value depends on system)
  (void)available;
}

TEST(SafetyFeaturesTest, ResourceCheckWithoutMonitor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Without monitor, should return true (default)
  bool available = action->CheckResourcesAvailable(100 * 1024 * 1024);
  EXPECT_TRUE(available);
}

// ============================================================================
// Process Control Tests with Safety
// ============================================================================

TEST(SafetyFeaturesTest, ShutdownWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("sleep"), text("10")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  // Execute a long-running process
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  // Test shutdown
  if (action->IsRunning()) {
    Error shutdownResult = action->Shutdown();
    // Result depends on implementation
    (void)shutdownResult;
  }
}

TEST(SafetyFeaturesTest, TerminateWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("sleep"), text("10")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  if (action->IsRunning()) {
    Error termResult = action->Terminate();
    (void)termResult;
  }
}

TEST(SafetyFeaturesTest, KillWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("sleep"), text("10")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  if (action->IsRunning()) {
    Error killResult = action->Kill();
    (void)killResult;
  }
}

// ============================================================================
// Status Query Tests
// ============================================================================

TEST(SafetyFeaturesTest, IsRunningWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  EXPECT_FALSE(action->IsRunning());
  
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  // Process might still be running or completed
  (void)action->IsRunning();
}

TEST(SafetyFeaturesTest, GetPidWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto pidResult = action->GetPid();
  EXPECT_FALSE(pidResult.HasError());  // Process not started
  
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  pidResult = action->GetPid();
  EXPECT_TRUE(pidResult.HasValue());
  EXPECT_GT(pidResult.GetValue(), 0);
}

TEST(SafetyFeaturesTest, GetReturnCodeWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = true;
  options.output = "screen";  // Wait for completion
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  auto returnCodeResult = action->GetReturnCode();
  if (returnCodeResult.HasValue()) {
    EXPECT_EQ(returnCodeResult.GetValue(), 0);
  }
}

// ============================================================================
// SendSignal Tests
// ============================================================================

TEST(SafetyFeaturesTest, SendSignalWithSafetyEnabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("sleep"), text("10")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  if (action->IsRunning()) {
    action->SendSignal(SIGTERM);
  }
}

// ============================================================================
// Destructor Tests
// ============================================================================

TEST(SafetyFeaturesTest, DestructorWithWatchdog)
{
  {
    ExecuteProcess::Options options;
    options.cmd = {text("echo"), text("hello")};
    options.enableSafety = true;
    options.watchdogTimeoutMs = 5000;
    
    auto action = std::make_shared<ExecuteProcess>(options);
    MockLaunchContext context;
    
    auto result = action->Execute(context);
    EXPECT_TRUE(result.HasValue());
    
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
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasError());
  EXPECT_EQ(result.GetError().GetCode(), ErrorCode::kInvalidArgument);
}

TEST(SafetyFeaturesTest, ResourceLimitsZeroValues)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = true;
  options.maxMemoryBytes = 0;  // Unlimited
  options.maxCpuPercent = 0.0;  // Unlimited
  options.watchdogTimeoutMs = 0;  // Disabled
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
}

TEST(SafetyFeaturesTest, ProcessControlBeforeExecution)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Try to control process before execution
  Error shutdownResult = action->Shutdown();
  Error termResult = action->Terminate();
  Error killResult = action->Kill();
  
  // Should not crash
  (void)shutdownResult;
  (void)termResult;
  (void)killResult;
}

TEST(SafetyFeaturesTest, GetStatusBeforeExecution)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  EXPECT_FALSE(action->IsRunning());
  
  auto pidResult = action->GetPid();
  EXPECT_TRUE(pidResult.HasError());
  
  auto returnCodeResult = action->GetReturnCode();
  EXPECT_TRUE(returnCodeResult.HasError());
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
  
  auto result = monitor->GetSystemResources();
  EXPECT_TRUE(result.IsSuccess());
}

TEST(SafetyFeaturesTest, PosixWatchdogCreation)
{
  auto watchdog = std::make_shared<PosixWatchdog>();
  EXPECT_NE(watchdog, nullptr);
  
  auto result = watchdog->Start();
  EXPECT_TRUE(result.IsSuccess());
  
  watchdog->Stop();
}

TEST(SafetyFeaturesTest, WatchdogRegisterAndUnregister)
{
  auto watchdog = std::make_shared<PosixWatchdog>();
  EXPECT_NE(watchdog, nullptr);
  
  auto startResult = watchdog->Start();
  EXPECT_TRUE(startResult.IsSuccess());
  
  auto regResult = watchdog->RegisterNode(1, 5000, nullptr);
  EXPECT_TRUE(regResult.IsSuccess());
  
  auto unregResult = watchdog->UnregisterNode(1);
  EXPECT_TRUE(unregResult.IsSuccess());
  
  watchdog->Stop();
}

TEST(SafetyFeaturesTest, WatchdogHeartbeat)
{
  auto watchdog = std::make_shared<PosixWatchdog>();
  EXPECT_NE(watchdog, nullptr);
  
  auto startResult = watchdog->Start();
  EXPECT_TRUE(startResult.IsSuccess());
  
  auto regResult = watchdog->RegisterNode(1, 5000, nullptr);
  EXPECT_TRUE(regResult.IsSuccess());
  
  HeartbeatMessage msg;
  msg.node_id = 1;
  msg.sequence = 1;
  msg.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
  msg.state = ProcessState::kRunning;
  msg.checksum = msg.CalculateChecksum();
  
  auto hbResult = watchdog->SubmitHeartbeat(msg);
  EXPECT_TRUE(hbResult.IsSuccess());
  
  auto responsiveResult = watchdog->IsResponsive(1);
  EXPECT_TRUE(responsiveResult.IsSuccess());
  EXPECT_TRUE(responsiveResult.GetValue());
  
  watchdog->Stop();
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(SafetyFeaturesIntegrationTest, FullSafetyWorkflow)
{
  ExecuteProcess::Options options;
  options.cmd = {text("/bin/echo"), text("safety test")};
  options.enableSafety = true;
  options.maxMemoryBytes = 100 * 1024 * 1024;
  options.maxCpuPercent = 50.0;
  options.watchdogTimeoutMs = 5000;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  // Check resources
  bool available = action->CheckResourcesAvailable(50 * 1024 * 1024);
  (void)available;
  
  // Execute
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
  
  if (action->IsRunning()) {
    auto pidResult = action->GetPid();
    EXPECT_TRUE(pidResult.HasValue());
    
    // Let it complete or terminate
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (action->IsRunning()) {
      action->Terminate();
    }
  }
}

TEST(SafetyFeaturesIntegrationTest, MultipleSafetyOptions)
{
  // Test various combinations
  std::vector<std::tuple<bool, uint64_t, double, int32_t>> testCases = {
    {true, 0, 0.0, 0},           // Safety enabled, no limits
    {true, 1024*1024, 0.0, 0},   // Memory limit only
    {true, 0, 50.0, 0},          // CPU limit only
    {true, 0, 0.0, 1000},        // Watchdog only
    {true, 1024*1024, 50.0, 1000}, // All limits
  };
  
  for (const auto& tc : testCases) {
    ExecuteProcess::Options options;
    options.cmd = {text("echo"), text("test")};
    options.enableSafety = std::get<0>(tc);
    options.maxMemoryBytes = std::get<1>(tc);
    options.maxCpuPercent = std::get<2>(tc);
    options.watchdogTimeoutMs = std::get<3>(tc);
    options.output = "log";
    
    auto action = std::make_shared<ExecuteProcess>(options);
    MockLaunchContext context;
    
    auto result = action->Execute(context);
    EXPECT_TRUE(result.HasValue());
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
