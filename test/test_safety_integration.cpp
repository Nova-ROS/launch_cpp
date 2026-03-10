/**
 * @file test_safety_integration.cpp
 * @brief Unit tests for safety architecture integration
 * 
 * Tests the integration of OSAL safety components with launch_cpp.
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <unordered_map>
#include "launch_cpp/actions/execute_process.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"
#include "launch_cpp/safety/osal.hpp"
#include "launch_cpp/event.hpp"
#include "launch_cpp/event_handler.hpp"
#include "launch_cpp/error_code.hpp"

using namespace launch_cpp;
using namespace launch_cpp;

// Mock LaunchContext for testing
class MockLaunchContext : public LaunchContext
{
 public:
  void register_event_handler(const EventHandlerPtr&) override {}
  void unregister_event_handler(const EventHandler*) override {}
  const EventHandlerVector& get_event_handlers() const override { return handlers_; }
  void set_launch_configuration(const std::string& key, const std::string& value) override
  {
    configs_[key] = value;
  }
  Result<std::string> get_launch_configuration(const std::string& key) const override
  {
    auto it = configs_.find(key);
    if (it == configs_.end()) {
      return Result<std::string>(Error(ErrorCode::kInvalidArgument, "Not found"));
    }
    return Result<std::string>(it->second);
  }
  bool has_launch_configuration(const std::string& key) const override
  {
    return configs_.find(key) != configs_.end();
  }
  std::string get_environment_variable(const std::string&) const override { return ""; }
  void set_environment_variable(const std::string&, const std::string&) override {}
  void emit_event(EventPtr) override {}
  void set_current_launch_file(const std::string& path) override { currentLaunchFile_ = path; }
  std::string get_current_launch_file() const override { return currentLaunchFile_; }

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
  EXPECT_FALSE(options.enable_safety);
  EXPECT_EQ(options.max_memory_bytes, 0u);
  EXPECT_DOUBLE_EQ(options.max_cpu_percent, 0.0);
  EXPECT_EQ(options.watchdog_timeout_ms, 0);
}

TEST(SafetyIntegrationTest, SafetyOptionsCanBeSet)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enable_safety = true;
  options.max_memory_bytes = 512 * 1024 * 1024;  // 512MB
  options.max_cpu_percent = 50.0;
  options.watchdog_timeout_ms = 5000;
  
  EXPECT_TRUE(options.enable_safety);
  EXPECT_EQ(options.max_memory_bytes, 512u * 1024 * 1024);
  EXPECT_DOUBLE_EQ(options.max_cpu_percent, 50.0);
  EXPECT_EQ(options.watchdog_timeout_ms, 5000);
}

// ============================================================================
// ExecuteProcess with Safety Tests
// ============================================================================

TEST(SafetyIntegrationTest, CreateExecuteProcessWithSafety)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
}

TEST(SafetyIntegrationTest, CreateExecuteProcessWithoutSafety)
{
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enable_safety = false;
  
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
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Create and set mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  action->set_process_executor(mockExecutor);
  
  // Verify it doesn't crash
  EXPECT_NE(action, nullptr);
}

TEST(SafetyIntegrationTest, ExecuteWithMockExecutor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.output = "log";  // Don't wait for output
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Configure mock to return success
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  action->set_process_executor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->execute(context);
  
  EXPECT_TRUE(result.has_value());
  
  // Verify PID is from mock
  auto pidResult = action->get_pid();
  EXPECT_TRUE(pidResult.has_value());
  EXPECT_EQ(pidResult.get_value(), 1234);
}

TEST(SafetyIntegrationTest, ExecuteWithMockExecutorFailure)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Configure mock to return failure
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(
        OsalStatus::kError,
        "Mock execution failed");
    });
  
  action->set_process_executor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->execute(context);
  
  EXPECT_TRUE(result.has_error());
}

// ============================================================================
// Resource Monitor Tests
// ============================================================================

TEST(SafetyIntegrationTest, SetResourceMonitor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Create and set resource monitor
  auto monitor = std::make_shared<PosixResourceMonitor>();
  action->set_resource_monitor(monitor);
  
  EXPECT_NE(action, nullptr);
}

TEST(SafetyIntegrationTest, CheckResourcesAvailableWithoutMonitor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Without monitor, should return true (assume available)
  EXPECT_TRUE(action->check_resources_available(100 * 1024 * 1024));
}

TEST(SafetyIntegrationTest, CheckResourcesAvailableWithMockMonitor)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Create mock monitor that returns false (resources unavailable)
  class MockResourceMonitor : public ResourceMonitor
  {
   public:
    OsalResult<SystemResources> get_system_resources() override
    {
      SystemResources res;
      return OsalResult<SystemResources>(res);
    }
    
    OsalResult<ResourceUsage> get_process_resources(ProcessId) override
    {
      ResourceUsage usage;
      return OsalResult<ResourceUsage>(usage);
    }
    
    OsalResult<bool> are_resources_available(uint64_t) override
    {
      return OsalResult<bool>(false);  // Simulate unavailable
    }
    
    OsalResult<void> set_resource_limits(ProcessId, uint64_t, double) override
    {
      return OsalResult<void>();
    }
    
    void register_threshold_callback(double, std::function<void(const SystemResources&)>) override {}
  };
  
  auto mockMonitor = std::make_shared<MockResourceMonitor>();
  action->set_resource_monitor(mockMonitor);
  
  // Should return false now
  EXPECT_FALSE(action->check_resources_available(100 * 1024 * 1024));
}

// ============================================================================
// Watchdog Tests
// ============================================================================

TEST(SafetyIntegrationTest, SetWatchdog)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Create and set watchdog
  auto watchdog = std::make_shared<PosixWatchdog>();
  action->set_watchdog(watchdog);
  
  EXPECT_NE(action, nullptr);
}

// ============================================================================
// Safety Execution Integration Tests
// ============================================================================

TEST(SafetyIntegrationTest, ExecuteWithResourceCheck)
{
  ExecuteProcess::Options options;
  options.cmd = {text("/bin/echo"), text("hello")};
  options.enable_safety = true;
  options.output = "log";
  
  // Create action with safety enabled
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Set up mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  bool executeCalled = false;
  mockExecutor->set_execute_callback(
    [&executeCalled](const CommandLine& cmd, const ProcessOptions&) -> OsalResult<ProcessId> {
      executeCalled = true;
      EXPECT_EQ(cmd.program, "/bin/echo");
      EXPECT_EQ(cmd.arguments.size(), 1u);
      EXPECT_EQ(cmd.arguments[0], "hello");
      return OsalResult<ProcessId>(1234);
    });
  
  action->set_process_executor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->execute(context);
  
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(executeCalled);
}

TEST(SafetyIntegrationTest, ProcessControlWithSafety)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.sigterm_timeout = 2;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Set up mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  
  // Track which methods are called
  bool terminateCalled = false;
  bool killCalled = false;
  
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->set_terminate_callback(
    [&terminateCalled](ProcessId, std::chrono::milliseconds) -> OsalResult<void> {
      terminateCalled = true;
      return OsalResult<void>();
    });
  
  mockExecutor->set_kill_callback(
    [&killCalled](ProcessId) -> OsalResult<void> {
      killCalled = true;
      return OsalResult<void>();
    });
  
  mockExecutor->set_is_running_callback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(true);
    });
  
  action->set_process_executor(mockExecutor);
  
  // First execute to set up the process
  MockLaunchContext context;
  options.output = "log";
  auto execResult = action->execute(context);
  EXPECT_TRUE(execResult.has_value());
  
  // Test terminate
  auto termResult = action->terminate();
  EXPECT_TRUE(termResult.is_success());
  EXPECT_TRUE(terminateCalled);
  
  // Test kill
  auto killResult = action->kill();
  EXPECT_TRUE(killResult.is_success());
  EXPECT_TRUE(killCalled);
}

TEST(SafetyIntegrationTest, GetStatusWithSafety)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Set up mock executor
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->set_is_running_callback(
    [](ProcessId) -> OsalResult<bool> {
      return OsalResult<bool>(true);
    });
  
  action->set_process_executor(mockExecutor);
  
  MockLaunchContext context;
  auto result = action->execute(context);
  EXPECT_TRUE(result.has_value());
  
  // Check status
  EXPECT_TRUE(action->is_running());
  
  // Check PID
  auto pidResult = action->get_pid();
  EXPECT_TRUE(pidResult.has_value());
  EXPECT_EQ(pidResult.get_value(), 1234);
}

// ============================================================================
// Backward Compatibility Tests
// ============================================================================

TEST(SafetyIntegrationTest, BackwardCompatibilityDisabled)
{
  // Test that safety disabled works exactly like before
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("hello")};
  options.enable_safety = false;  // Explicitly disabled
  
  auto action = std::make_shared<ExecuteProcess>(options);
  EXPECT_NE(action, nullptr);
  
  // All operations should still work
  EXPECT_FALSE(action->is_running());
  
  auto pidResult = action->get_pid();
  EXPECT_FALSE(pidResult.has_value());  // Process not started
}

TEST(SafetyIntegrationTest, SendSignalWithSafety)
{
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enable_safety = true;
  options.output = "log";
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  bool signalCalled = false;
  int32_t receivedSignal = 0;
  
  mockExecutor->set_execute_callback(
    [](const CommandLine&, const ProcessOptions&) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->set_send_signal_callback(
    [&signalCalled, &receivedSignal](ProcessId, int32_t sig) -> OsalResult<void> {
      signalCalled = true;
      receivedSignal = sig;
      return OsalResult<void>();
    });
  
  action->set_process_executor(mockExecutor);
  
  // Set mock resource monitor to avoid resource check failures
  auto mockMonitor = std::make_shared<MockResourceMonitor>();
  action->set_resource_monitor(mockMonitor);
  
  MockLaunchContext context;
  auto result = action->execute(context);
  EXPECT_TRUE(result.has_value());
  
  // Send SIGUSR1
  action->send_signal(SIGUSR1);
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
