/**
 * @file test_safety_integration.cpp
 * @brief Unit tests for safety architecture integration
 * 
 * Tests the integration of OSAL safety components with cpp_launch.
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <unordered_map>
#include "cpp_launch/actions/execute_process.hpp"
#include "cpp_launch/launch_context.hpp"
#include "cpp_launch/substitutions/text_substitution.hpp"
#include "cpp_launch/safety/osal.hpp"
#include "cpp_launch/event.hpp"
#include "cpp_launch/event_handler.hpp"
#include "cpp_launch/error_code.hpp"

using namespace cpp_launch;
using namespace ara::exec;

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

// Helper to create text substitution
SubstitutionPtr text(const std::string& str)
{
  return std::make_shared<TextSubstitution>(str);
}

// ============================================================================
// Safety Options Tests
// ============================================================================

TEST(SafetyIntegrationTest, DefaultSafetyDisabled)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  
  // Default safety should be disabled
  EXPECT_FALSE(options.enableSafety);
  EXPECT_EQ(options.maxMemoryBytes, 0u);
  EXPECT_DOUBLE_EQ(options.maxCpuPercent, 0.0);
  EXPECT_EQ(options.watchdogTimeoutMs, 0);
}

TEST(SafetyIntegrationTest, SafetyOptionsCanBeSet)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = true;
  options.maxMemoryBytes = 512 * 1024 * 1024;  // 512MB
  options.maxCpuPercent = 50.0;
  options.watchdogTimeoutMs = 5000;
  
  EXPECT_TRUE(options.enableSafety);
  EXPECT_EQ(options.maxMemoryBytes, 512u * 1024 * 1024);
  EXPECT_DOUBLE_EQ(options.maxCpuPercent, 50.0);
  EXPECT_EQ(options.watchdogTimeoutMs, 5000);
}

// ============================================================================
// ExecuteProcess with Safety Tests
// ============================================================================

TEST(SafetyIntegrationTest, CreateExecuteProcessWithSafety)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
}

TEST(SafetyIntegrationTest, CreateExecuteProcessWithoutSafety)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = false;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
}

// ============================================================================
// Mock Process Executor Tests
// ============================================================================

TEST(SafetyIntegrationTest, SetMockProcessExecutor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Create and set mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  action->SetProcessExecutor(mockExecutor);
  
  // Verify it doesn't crash
  EXPECT_NE(action, nullptr);
}

TEST(SafetyIntegrationTest, ExecuteWithMockExecutor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.output = "log";  // Don't wait for output
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Configure mock to return success
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->SetExecuteCallback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  action->SetProcessExecutor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  
  EXPECT_TRUE(result.IsSuccess());
  
  // Verify PID is from mock
  auto pidResult = action->GetPid();
  EXPECT_TRUE(pidResult.IsSuccess());
  EXPECT_EQ(pidResult.GetValue(), 1234);
}

TEST(SafetyIntegrationTest, ExecuteWithMockExecutorFailure)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Configure mock to return failure
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
  
  EXPECT_FALSE(result.IsSuccess());
}

// ============================================================================
// Resource Monitor Tests
// ============================================================================

TEST(SafetyIntegrationTest, SetResourceMonitor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Create and set resource monitor
  auto monitor = std::make_shared<PosixResourceMonitor>();
  action->SetResourceMonitor(monitor);
  
  EXPECT_NE(action, nullptr);
}

TEST(SafetyIntegrationTest, CheckResourcesAvailableWithoutMonitor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Without monitor, should return true (assume available)
  EXPECT_TRUE(action->CheckResourcesAvailable(100 * 1024 * 1024));
}

TEST(SafetyIntegrationTest, CheckResourcesAvailableWithMockMonitor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Create mock monitor that returns false (resources unavailable)
  class MockResourceMonitor : public ResourceMonitor
  {
   public:
    OsalResult<SystemResources> GetSystemResources() override
    {
      SystemResources res;
      return OsalResult<SystemResources>(res);
    }
    
    OsalResult<ResourceUsage> GetProcessResources(ProcessId) override
    {
      ResourceUsage usage;
      return OsalResult<ResourceUsage>(usage);
    }
    
    OsalResult<bool> AreResourcesAvailable(uint64_t) override
    {
      return OsalResult<bool>(false);  // Simulate unavailable
    }
    
    OsalResult<void> SetResourceLimits(ProcessId, uint64_t, double) override
    {
      return OsalResult<void>();
    }
    
    void RegisterThresholdCallback(double, std::function<void(const SystemResources&)>) override {}
  };
  
  auto mockMonitor = std::make_shared<MockResourceMonitor>();
  action->SetResourceMonitor(mockMonitor);
  
  // Should return false now
  EXPECT_FALSE(action->CheckResourcesAvailable(100 * 1024 * 1024));
}

// ============================================================================
// Watchdog Tests
// ============================================================================

TEST(SafetyIntegrationTest, SetWatchdog)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Create and set watchdog
  auto watchdog = std::make_shared<PosixWatchdog>();
  action->SetWatchdog(watchdog);
  
  EXPECT_NE(action, nullptr);
}

// ============================================================================
// Safety Execution Integration Tests
// ============================================================================

TEST(SafetyIntegrationTest, ExecuteWithResourceCheck)
{
  ExecuteProcess::Options options;
  options.cmd = {text("/bin/echo"), text("hello")};
  options.enableSafety = true;
  options.output = "log";
  
  // Create action with safety enabled
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Set up mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  bool executeCalled = false;
  mockExecutor->SetExecuteCallback(
    [&executeCalled](const CommandLine& cmd, const ProcessOptions&) -> OsalResult<ProcessId> {
      executeCalled = true;
      EXPECT_EQ(cmd.program, "/bin/echo");
      EXPECT_EQ(cmd.arguments.size(), 1u);
      EXPECT_EQ(cmd.arguments[0], "hello");
      return OsalResult<ProcessId>(1234);
    });
  
  action->SetProcessExecutor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  
  EXPECT_TRUE(result.IsSuccess());
  EXPECT_TRUE(executeCalled);
}

TEST(SafetyIntegrationTest, ProcessControlWithSafety)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.sigtermTimeout = 2;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Set up mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  
  // Track which methods are called
  bool terminateCalled = false;
  bool killCalled = false;
  
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
  
  // First execute to set up the process
  MockLaunchContext context;
  options.output = "log";
  auto execResult = action->Execute(context);
  EXPECT_TRUE(execResult.IsSuccess());
  
  // Test terminate
  auto termResult = action->Terminate();
  EXPECT_TRUE(termResult.IsSuccess());
  EXPECT_TRUE(terminateCalled);
  
  // Test kill
  auto killResult = action->Kill();
  EXPECT_TRUE(killResult.IsSuccess());
  EXPECT_TRUE(killCalled);
}

TEST(SafetyIntegrationTest, GetStatusWithSafety)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Set up mock executor
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
  EXPECT_TRUE(result.IsSuccess());
  
  // Check status
  EXPECT_TRUE(action->IsRunning());
  
  // Check PID
  auto pidResult = action->GetPid();
  EXPECT_TRUE(pidResult.IsSuccess());
  EXPECT_EQ(pidResult.GetValue(), 1234);
}

// ============================================================================
// Backward Compatibility Tests
// ============================================================================

TEST(SafetyIntegrationTest, BackwardCompatibilityDisabled)
{
  // Test that safety disabled works exactly like before
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enableSafety = false;  // Explicitly disabled
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
  
  // All operations should still work
  EXPECT_FALSE(action->IsRunning());
  
  auto pidResult = action->GetPid();
  EXPECT_FALSE(pidResult.IsSuccess());  // Process not started
}

TEST(SafetyIntegrationTest, SendSignalWithSafety)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  bool signalCalled = false;
  int32_t receivedSignal = 0;
  
  mockExecutor->SetExecuteCallback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->SetSendSignalCallback(
    [&signalCalled, &receivedSignal](ProcessId, int32_t sig) -> OsalResult<void> {
      signalCalled = true;
      receivedSignal = sig;
      return OsalResult<void>();
    });
  
  action->SetProcessExecutor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  EXPECT_TRUE(result.IsSuccess());
  
  // Send SIGUSR1
  action->SendSignal(SIGUSR1);
  EXPECT_TRUE(signalCalled);
  EXPECT_EQ(receivedSignal, SIGUSR1);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
