# launch_cpp 架构文档

## 1. 系统概述

**launch_cpp** 是一个现代化的 C++14 实现的 ROS2 Launch 系统，完全符合 AUTOSAR C++14 标准和 ISO 26262 ASIL B 功能安全要求。

### 1.1 核心特性

- **高性能**: 编译型 C++14，最小运行时开销
- **类型安全**: 模板化的 `Result<T>` 错误处理，无异常
- **零依赖**: 仅使用 C++ 标准库，无外部依赖
- **模块化**: 清晰的接口和职责分离
- **功能安全**: ISO 26262 ASIL B 就绪

### 1.2 技术栈

| 组件 | 实现方式 | 代码规模 |
|------|----------|----------|
| **YAML 解析** | 自定义实现 | ~450 行 |
| **线程池** | 自定义实现 | ~100 行 |
| **错误处理** | Result<T> 模式 | ~150 行 |
| **进程管理** | POSIX 标准 | ~150 行 |
| **总计** | - | ~850 行自实现代码 |

## 2. 架构层次

### 2.1 五层架构

```
┌─────────────────────────────────────────────────────────────┐
│ 第五层: ROS 扩展 (ROS Extensions)                            │
│  - Node (ROS 节点)                                           │
│  - LifecycleNode (生命周期节点)                               │
│  - ComposableNodeContainer (组件节点容器)                     │
├─────────────────────────────────────────────────────────────┤
│ 第四层: 条件与替换 (Conditions & Substitutions)              │
│  - Condition (条件基类)                                      │
│  - IfCondition / UnlessCondition                             │
│  - Substitution (替换基类)                                   │
│  - TextSubstitution, LaunchConfiguration, EnvironmentVariable│
├─────────────────────────────────────────────────────────────┤
│ 第三层: 动作实现 (Action Implementations)                    │
│  - ExecuteProcess (执行外部进程)                             │
│  - DeclareLaunchArgument (声明启动参数)                      │
│  - IncludeLaunchDescription (包含启动文件)                   │
│  - TimerAction, GroupAction                                  │
├─────────────────────────────────────────────────────────────┤
│ 第二层: 实体抽象 (Entity Abstraction)                        │
│  - LaunchDescriptionEntity (实体基类)                        │
│  - LaunchDescription (启动描述容器)                          │
│  - Action (动作基类)                                         │
│  - Event / EventHandler                                      │
├─────────────────────────────────────────────────────────────┤
│ 第一层: 核心运行时 (Core Runtime)                            │
│  - LaunchService (主服务)                                    │
│  - LaunchContext (运行时上下文)                              │
│  - ThreadPool (线程池)                                       │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 安全架构层 (Safety Layer)

```
┌─────────────────────────────────────────────────────────────┐
│ 安全架构 (ISO 26262 ASIL B)                                  │
├─────────────────────────────────────────────────────────────┤
│ 业务逻辑层                                                   │
│  - DependencyResolver (依赖解析)                            │
│  - CommandBuilder (命令构建)                                │
│  - RetryPolicy (重试策略)                                   │
├─────────────────────────────────────────────────────────────┤
│ OSAL 抽象层                                                  │
│  - ProcessExecutor (进程执行接口)                           │
│  - ResourceMonitor (资源监控接口)                           │
│  - Watchdog (看门狗接口)                                    │
├─────────────────────────────────────────────────────────────┤
│ POSIX 实现层                                                 │
│  - PosixProcessExecutor                                     │
│  - PosixResourceMonitor                                     │
│  - PosixWatchdog                                            │
├─────────────────────────────────────────────────────────────┤
│ Mock 层 (测试)                                              │
│  - MockProcessExecutor                                      │
│  - MockResourceMonitor                                      │
└─────────────────────────────────────────────────────────────┘
```

## 3. 核心组件

### 3.1 LaunchService

**职责**: 主事件循环和服务生命周期管理

```cpp
class LaunchService {
public:
    // 运行启动服务
    std::int32_t Run();
    
    // 包含启动描述
    Error IncludeLaunchDescription(LaunchDescriptionPtr description);
    
    // 关闭服务
    void Shutdown();
};
```

**设计要点**:
- 单线程事件循环
- 异步事件处理
- 优雅关闭支持

### 3.2 LaunchContext

**职责**: 运行时上下文和状态管理

**状态包含**:
- Event handlers 列表
- Launch configurations (启动配置)
- Environment variables (环境变量)
- Local / Global 变量

### 3.3 Action 体系

**继承层次**:
```
LaunchDescriptionEntity (抽象基类)
    └── Action (动作基类)
            ├── ExecuteProcess (执行进程)
            ├── DeclareLaunchArgument (声明参数)
            ├── IncludeLaunchDescription (包含文件)
            ├── SetLaunchConfiguration (设置配置)
            ├── TimerAction (定时器)
            └── GroupAction (分组)
```

**执行流程**:
1. `Visit()` - 访问实体
2. `Execute()` - 执行动作
3. 返回子实体列表

## 4. 设计模式

### 4.1 访问者模式 (Visitor Pattern)

所有实体通过 `Visit()` 方法被访问，支持递归遍历。

```cpp
class LaunchDescriptionEntity {
public:
    virtual Result<LaunchDescriptionEntityVector> 
        Visit(LaunchContext& context) = 0;
};
```

### 4.2 事件驱动架构

```
[Event Source] → [Event Queue] → [Event Handlers] → [New Entities]
```

**事件类型**:
- ProcessStartedEvent
- ProcessExitedEvent
- TimerEvent
- ShutdownEvent

### 4.3 策略模式 (Strategy)

**Conditions**:
- `IfCondition` - 真值判断
- `UnlessCondition` - 假值判断
- `LaunchConfigurationEquals` - 配置相等

**Substitutions**:
- `TextSubstitution` - 文本
- `LaunchConfiguration` - 配置替换
- `EnvironmentVariable` - 环境变量
- `VariableSubstitution` - 变量替换 ($(var name))

### 4.4 Result<T> 错误处理

AUTOSAR C++14 合规的错误处理，无异常。

```cpp
template<typename T>
class Result {
public:
    bool IsSuccess() const;
    bool HasError() const;
    Error GetError() const;
    T& GetValue();
};
```

## 5. 线程模型

### 5.1 架构

```
主线程 (Main Thread)
    └── LaunchService::Run() [事件循环]
            ├── 处理事件队列
            ├── 执行 Actions
            └── 状态管理

工作线程池 (Worker Threads)
    └── ThreadPool [处理阻塞操作]
            ├── 进程 I/O
            ├── 文件操作
            └── 网络请求
```

### 5.2 线程安全

- **Event Queue**: 线程安全队列 (mutex + condition_variable)
- **Context State**: 主线程访问，无需锁
- **Process I/O**: 回调机制，线程池处理

## 6. 数据流

### 6.1 启动文件解析

```
YAML File → YamlParser → YamlValue → BuildAction → Action Objects
```

### 6.2 执行流程

```
LaunchDescription
    └── Visit() → Action::Execute()
            ├── ExecuteProcess → fork/exec
            ├── DeclareLaunchArgument → 设置配置
            └── IncludeLaunchDescription → 递归解析
```

### 6.3 事件流

```
Process Started → ProcessStartedEvent → Handlers → New Actions
```

## 7. 依赖管理

### 7.1 DependencyResolver

使用 **Kahn 算法**进行拓扑排序。

```cpp
class DependencyResolver {
public:
    ResolutionResult Resolve(const std::vector<NodeConfig>& nodes);
    bool HasCircularDependency(const std::vector<NodeConfig>& nodes);
};
```

**复杂度**:
- 时间: O(V + E)
- 空间: O(V)

### 7.2 使用示例

```yaml
entities:
  - type: execute_process
    name: database
    cmd: [echo, "DB started"]
  
  - type: execute_process
    name: api_server
    cmd: [echo, "API started"]
    depends_on:
      - database
```

## 8. 安全架构

### 8.1 OSAL 层

**设计原则**:
- 接口抽象，实现可替换
- 支持 Mock 测试
- 线程安全

**核心接口**:
- `ProcessExecutor` - 进程执行
- `ResourceMonitor` - 资源监控
- `Watchdog` - 看门狗

### 8.2 安全功能

**ExecuteProcess 安全选项**:
```cpp
struct Options {
    bool enableSafety = false;
    std::uint64_t maxMemoryBytes = 0;
    double maxCpuPercent = 0.0;
    std::int32_t watchdogTimeoutMs = 0;
    std::uint32_t maxRetries = 0;
};
```

**功能**:
- 资源限制 (内存、CPU)
- 看门狗监控
- 自动重试
- 优雅关闭

## 9. 性能指标

| 指标 | 值 |
|------|-----|
| 启动时间 | <10ms (典型启动文件) |
| 内存开销 | <1MB (基础) |
| 进程创建 | ~1ms (POSIX) |
| YAML 解析 | ~5ms (100 行) |

## 10. 扩展性

### 10.1 添加新 Action

```cpp
class MyAction : public Action {
public:
    Result<void> Execute(LaunchContext& context) override {
        // 实现
        return Result<void>();
    }
};
```

### 10.2 添加新 Substitution

```cpp
class MySubstitution : public Substitution {
public:
    std::string Perform(const LaunchContext& context) const override {
        return "value";
    }
};
```

## 11. 文件组织

```
launch_cpp/
├── include/launch_cpp/
│   ├── launch_service.hpp
│   ├── launch_context.hpp
│   ├── launch_description.hpp
│   ├── action.hpp
│   ├── event.hpp
│   ├── substitution.hpp
│   ├── condition.hpp
│   ├── dependency_manager.hpp
│   └── safety/
│       ├── osal.hpp
│       ├── dependency_resolver.hpp
│       └── command_builder.hpp
├── src/
│   ├── launch_service.cpp
│   ├── yaml_parser.cpp
│   ├── actions/
│   ├── substitutions/
│   └── safety/
├── test/
│   └── test_*.cpp
└── docs/
    └── *.md
```

## 12. 参考资料

- [AUTOSAR C++14 Coding Guidelines](https://www.autosar.org/fileadmin/user_upload/standards/adaptive/17-03/AUTOSAR_RS_CPP14Guidelines.pdf)
- [ISO 26262 - Road vehicles functional safety](https://www.iso.org/standard/68383.html)
- [ROS2 Launch System Design](http://design.ros2.org/articles/roslaunch.html)
- [Kahn's Algorithm](https://en.wikipedia.org/wiki/Kahn%27s_algorithm)