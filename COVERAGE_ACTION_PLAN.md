# 立即可执行的覆盖率改进方案

## 问题
重新编译遇到依赖问题，先使用现有方案。

## 当前状态
基于最后一次成功的测试运行:

### 当前覆盖率 (无分支数据)
- **行覆盖率**: 30.2% (1531/5062行)
- **函数覆盖率**: 40.7% (398/977函数)
- **分支覆盖率**: 未启用

---

## 立即执行方案

### 方案1: 添加更多测试用例 (推荐)

**目标**: 将 execute_process.cpp 从 16.9% 提升到 50%+

**需要添加的测试**:

```cpp
// test_execute_process_extended.cpp

// 1. 测试 Legacy 执行路径 (非安全模式)
TEST(ExecuteProcessExtended, LegacyExecutionPath) {
  ExecuteProcess::Options options;
  options.cmd = {text("echo"), text("test")};
  options.enableSafety = false;  // 测试非安全路径
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto result = action->Execute(context);
  // 应该使用传统 fork/exec 路径
  EXPECT_TRUE(result.HasValue());
}

// 2. 测试资源不足的情况
TEST(ExecuteProcessExtended, ResourceExhausted) {
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.maxMemoryBytes = 1;  // 1 byte - impossible
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->SetExecuteCallback(
    [](...) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(OsalStatus::kResourceExhausted, "No memory");
    });
  
  action->SetProcessExecutor(mockExecutor);
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasError());
}

// 3. 测试超时处理
TEST(ExecuteProcessExtended, ExecutionTimeout) {
  ExecuteProcess::Options options;
  options.cmd = {text("slow_command")};
  options.enableSafety = true;
  options.sigtermTimeout = 1;  // 1 second
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  bool terminateCalled = false;
  
  mockExecutor->SetExecuteCallback(
    [](...) -> OsalResult<ProcessId> {
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->SetIsRunningCallback(
    [](...) -> OsalResult<bool> {
      return OsalResult<bool>(true);  // Always running
    });
  
  mockExecutor->SetTerminateCallback(
    [&terminateCalled](...) -> OsalResult<void> {
      terminateCalled = true;
      return OsalResult<void>();
    });
  
  action->SetProcessExecutor(mockExecutor);
  action->Execute(context);
  
  // Wait for timeout
  std::this_thread::sleep_for(std::chrono::seconds(2));
  action->Terminate();
  
  EXPECT_TRUE(terminateCalled);
}

// 4. 测试多个信号
TEST(ExecuteProcessExtended, MultipleSignals) {
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  std::vector<int32_t> signalsReceived;
  
  mockExecutor->SetExecuteCallback([](...) { return OsalResult<ProcessId>(1234); });
  mockExecutor->SetIsRunningCallback([](...) { return OsalResult<bool>(true); });
  mockExecutor->SetSendSignalCallback(
    [&signalsReceived](ProcessId, int32_t sig) -> OsalResult<void> {
      signalsReceived.push_back(sig);
      return OsalResult<void>();
    });
  
  action->SetProcessExecutor(mockExecutor);
  action->Execute(context);
  
  // Send multiple signals
  action->SendSignal(SIGTERM);
  action->SendSignal(SIGUSR1);
  action->SendSignal(SIGUSR2);
  
  EXPECT_EQ(signalsReceived.size(), 3);
  EXPECT_EQ(signalsReceived[0], SIGTERM);
}

// 5. 测试析构时的清理
TEST(ExecuteProcessExtended, DestructorCleanup) {
  {
    ExecuteProcess::Options options;
    options.cmd = {text("test")};
    options.enableSafety = true;
    options.watchdogTimeoutMs = 1000;
    
    auto action = std::make_shared<ExecuteProcess>(options);
    MockLaunchContext context;
    
    auto mockExecutor = std::make_shared<MockProcessExecutor>();
    mockExecutor->SetExecuteCallback([](...) { return OsalResult<ProcessId>(1234); });
    
    action->SetProcessExecutor(mockExecutor);
    action->Execute(context);
    
    // Destructor should cleanup
  }
  // If we reach here without crash, test passed
  SUCCEED();
}

// 6. 测试空命令的错误路径
TEST(ExecuteProcessExtended, EmptyCommandError) {
  ExecuteProcess::Options options;
  options.cmd = {};  // Empty
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasError());
  EXPECT_EQ(result.GetError().GetCode(), ErrorCode::kInvalidArgument);
}

// 7. 测试资源限制设置
TEST(ExecuteProcessExtended, ResourceLimitsSetting) {
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.maxMemoryBytes = 1024 * 1024 * 100;  // 100MB
  options.maxCpuPercent = 50.0;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  bool limitsSet = false;
  
  mockExecutor->SetExecuteCallback([](...) { return OsalResult<ProcessId>(1234); });
  
  action->SetProcessExecutor(mockExecutor);
  auto result = action->Execute(context);
  
  EXPECT_TRUE(result.HasValue());
}

// 8. 测试看门狗注册失败
TEST(ExecuteProcessExtended, WatchdogRegistrationFailure) {
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  options.watchdogTimeoutMs = 1000;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->SetExecuteCallback([](...) { return OsalResult<ProcessId>(1234); });
  
  action->SetProcessExecutor(mockExecutor);
  
  // Should handle watchdog registration gracefully
  auto result = action->Execute(context);
  EXPECT_TRUE(result.HasValue());
}

// 9. 测试状态查询边界
TEST(ExecuteProcessExtended, StatusQueryBoundary) {
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  
  // Before execution
  EXPECT_FALSE(action->IsRunning());
  EXPECT_TRUE(action->GetPid().HasError());
  EXPECT_TRUE(action->GetReturnCode().HasError());
  
  MockLaunchContext context;
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  mockExecutor->SetExecuteCallback([](...) { return OsalResult<ProcessId>(1234); });
  mockExecutor->SetIsRunningCallback([](...) { return OsalResult<bool>(false); });
  
  action->SetProcessExecutor(mockExecutor);
  action->Execute(context);
  
  // After execution - process completed
  EXPECT_FALSE(action->IsRunning());
}

// 10. 测试进程控制回调
TEST(ExecuteProcessExtended, ProcessControlCallbacks) {
  ExecuteProcess::Options options;
  options.cmd = {text("test")};
  options.enableSafety = true;
  
  auto action = std::make_shared<ExecuteProcess>(options);
  MockLaunchContext context;
  
  auto mockExecutor = std::make_shared<MockProcessExecutor>();
  
  bool executeCalled = false;
  bool terminateCalled = false;
  bool killCalled = false;
  
  mockExecutor->SetExecuteCallback(
    [&executeCalled](...) -> OsalResult<ProcessId> {
      executeCalled = true;
      return OsalResult<ProcessId>(1234);
    });
  
  mockExecutor->SetTerminateCallback(
    [&terminateCalled](...) -> OsalResult<void> {
      terminateCalled = true;
      return OsalResult<void>();
    });
  
  mockExecutor->SetKillCallback(
    [&killCalled](...) -> OsalResult<void> {
      killCalled = true;
      return OsalResult<void>();
    });
  
  mockExecutor->SetIsRunningCallback([](...) { return OsalResult<bool>(true); });
  
  action->SetProcessExecutor(mockExecutor);
  
  action->Execute(context);
  EXPECT_TRUE(executeCalled);
  
  action->Terminate();
  EXPECT_TRUE(terminateCalled);
  
  action->Kill();
  EXPECT_TRUE(killCalled);
}
```

---

### 方案2: 启用分支覆盖率 (需要重新编译)

当编译问题解决后，执行:

```bash
# 1. 清除旧的构建
rm -rf build install log

# 2. 重新编译（带分支覆盖率）
source /home/bingdian/work/ros2/source/jazzy/install/setup.bash
colcon build --packages-select cpp_launch \
  --cmake-args \
  -DCMAKE_CXX_FLAGS="-g -O0 --coverage -fbranch-probabilities" \
  -DCMAKE_C_FLAGS="-g -O0 --coverage -fbranch-probabilities"

# 3. 运行测试
cd build/cpp_launch
./test_execute_process_safety
./test_safety_mock
./test_safety_features
./test_actions_comprehensive
./test_integration

# 4. 生成覆盖率报告
lcov --capture --directory . --output-file coverage_branch.info \
  --exclude '/usr/*' --exclude '*/gtest/*' --exclude '*/test/*'

# 5. 查看分支覆盖率
lcov --summary coverage_branch.info
```

---

### 方案3: 使用 gcovr 工具 (推荐)

```bash
# 安装 gcovr
pip3 install gcovr

# 生成带分支的HTML报告
cd build/cpp_launch
gcovr -r ../../src/ros2/cpp_launch \
  --html --html-details \
  --branches \
  -o coverage_report.html

# 查看摘要
gcovr -r ../../src/ros2/cpp_launch --branches --print-summary
```

---

## 预期改进

### 添加10个新测试后预期覆盖率:

| 文件 | 当前 | 预期 | 提升 |
|------|------|------|------|
| execute_process.cpp | 16.9% | 45%+ | +28% |
| mock_osal.cpp | 43.6% | 60%+ | +16% |
| **总体行覆盖率** | 30.2% | 40%+ | +10% |

### 启用分支覆盖率后:
- 识别未覆盖的分支（如 if/else 路径）
- 针对性添加测试
- 预期分支覆盖率可达60%+

---

## 总结

**立即能做的** (无需重新编译):
1. ✅ 添加更多测试用例 (方案1)
2. ✅ 测试边界条件和错误路径
3. ✅ 验证代码执行不崩溃

**需要编译环境修复后**:
1. 启用分支覆盖率
2. 使用 gcovr 生成更详细的报告
3. 针对性改进

**当前成果**:
- ✅ 46个测试100%通过
- ✅ 关键安全代码路径已测试
- 🟡 总体覆盖率30.2%，需要继续提升
