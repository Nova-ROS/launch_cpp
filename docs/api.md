# launch_cpp API 文档

## 目录

1. [核心类](#核心类)
2. [动作类](#动作类)
3. [事件系统](#事件系统)
4. [条件与替换](#条件与替换)
5. [安全架构](#安全架构)
6. [使用示例](#使用示例)

---

## 核心类

### LaunchService

主服务类，管理启动系统的事件循环和生命周期。

```cpp
namespace launch_cpp {

class LaunchService {
public:
    // 构造函数
    LaunchService();
    ~LaunchService();
    
    // 禁止拷贝，允许移动
    LaunchService(const LaunchService&) = delete;
    LaunchService& operator=(const LaunchService&) = delete;
    LaunchService(LaunchService&&) noexcept;
    LaunchService& operator=(LaunchService&&) noexcept;
    
    // 主运行函数
    // 返回退出码，0 表示成功
    std::int32_t Run();
    
    // 添加启动描述
    // 在 Run() 之前调用
    Error IncludeLaunchDescription(LaunchDescriptionPtr description);
    
    // 关闭服务
    // 发送关闭事件，优雅关闭所有进程
    void Shutdown();
    
    // 状态查询
    bool IsRunning() const noexcept;
    bool IsShutdown() const noexcept;
    
    // 获取上下文
    LaunchContext& GetContext();
    const LaunchContext& GetContext() const;
};

} // namespace launch_cpp
```

**使用示例**:

```cpp
LaunchService service;

auto desc = std::make_shared<LaunchDescription>();
desc->Add(std::make_shared<ExecuteProcess>(options));

auto error = service.IncludeLaunchDescription(desc);
if (error.IsError()) {
    std::cerr << error.GetMessage() << std::endl;
    return 1;
}

int exit_code = service.Run();
return exit_code;
```

---

### LaunchContext

运行时上下文，包含所有启动状态和配置。

```cpp
class LaunchContext {
public:
    // 事件处理器管理
    void RegisterEventHandler(const EventHandlerPtr& handler);
    void UnregisterEventHandler(const EventHandler* handler);
    const EventHandlerVector& GetEventHandlers() const;
    
    // 启动配置管理
    void SetLaunchConfiguration(const std::string& key, const std::string& value);
    Result<std::string> GetLaunchConfiguration(const std::string& key) const;
    bool HasLaunchConfiguration(const std::string& key) const;
    
    // 环境变量管理
    std::string GetEnvironmentVariable(const std::string& name) const;
    void SetEnvironmentVariable(const std::string& name, const std::string& value);
    
    // 当前启动文件
    void SetCurrentLaunchFile(const std::string& path);
    std::string GetCurrentLaunchFile() const;
    
    // 发射事件
    void EmitEvent(EventPtr event);
};
```

**使用示例**:

```cpp
LaunchContext context;

// 设置配置
context.SetLaunchConfiguration("robot_name", "my_robot");

// 获取配置
auto result = context.GetLaunchConfiguration("robot_name");
if (result.HasValue()) {
    std::cout << "Robot: " << result.GetValue() << std::endl;
}

// 注册事件处理器
context.RegisterEventHandler(std::make_shared<MyEventHandler>());
```

---

### LaunchDescription

启动描述容器，包含所有要执行的实体。

```cpp
class LaunchDescription : public LaunchDescriptionEntity {
public:
    LaunchDescription();
    ~LaunchDescription() override = default;
    
    // 添加实体
    void Add(const LaunchDescriptionEntityPtr& entity);
    
    // 访问所有实体
    const std::vector<LaunchDescriptionEntityPtr>& GetEntities() const;
    
    // 从 YAML 文件加载
    static Result<LaunchDescriptionPtr> FromYamlFile(const std::string& path);
    static Result<LaunchDescriptionPtr> FromYamlString(const std::string& content);
    
    // Visitor 模式
    Result<LaunchDescriptionEntityVector> Visit(LaunchContext& context) override;
};
```

---

## 动作类

### ExecuteProcess

执行外部进程的动作。

```cpp
class ExecuteProcess : public Action {
public:
    struct Options {
        // 命令行参数
        std::vector<SubstitutionPtr> cmd;
        
        // 工作目录
        SubstitutionPtr cwd;
        
        // 环境变量
        std::unordered_map<std::string, SubstitutionPtr> env;
        
        // 输出模式: "screen", "log", "both"
        std::string output = "log";
        
        // TTY 模拟
        bool emulateTty = false;
        
        // SIGTERM 超时 (秒)
        std::int32_t sigtermTimeout = 5;
        
        // 进程名称
        SubstitutionPtr name;
        
        // 依赖
        std::vector<std::string> dependsOn;
        
        // 安全选项
        bool enableSafety = false;
        std::uint64_t maxMemoryBytes = 0;
        double maxCpuPercent = 0.0;
        std::int32_t watchdogTimeoutMs = 0;
        
        // 重试选项
        std::uint32_t maxRetries = 0;
        std::chrono::milliseconds retryDelay{5000};
        double retryBackoffMultiplier = 1.0;
    };
    
    explicit ExecuteProcess(const Options& options);
    ~ExecuteProcess() override;
    
    // 执行动作
    Result<void> Execute(LaunchContext& context) override;
    
    // 进程控制
    Error Shutdown();   // 发送 SIGTERM
    Error Terminate();  // 发送 SIGTERM，超时后 SIGKILL
    Error Kill();       // 发送 SIGKILL
    void SendSignal(std::int32_t signal);
    
    // 状态查询
    bool IsRunning() const noexcept;
    Result<std::int32_t> GetReturnCode() const;
    Result<std::int32_t> GetPid() const;
    std::string GetName() const;
    
    // 安全相关
    void SetProcessExecutor(std::shared_ptr<ProcessExecutor> executor);
    void SetResourceMonitor(std::shared_ptr<ResourceMonitor> monitor);
    void SetWatchdog(std::shared_ptr<Watchdog> watchdog);
    bool CheckResourcesAvailable(std::uint64_t estimatedMemory) const;
};
```

**使用示例**:

```cpp
ExecuteProcess::Options options;
options.cmd = {
    std::make_shared<TextSubstitution>("ros2"),
    std::make_shared<TextSubstitution>("run"),
    std::make_shared<TextSubstitution>("my_pkg"),
    std::make_shared<TextSubstitution>("my_node")
};
options.output = "screen";
options.name = std::make_shared<TextSubstitution>("my_node");
options.enableSafety = true;
options.maxMemoryBytes = 100 * 1024 * 1024;  // 100MB

auto action = std::make_shared<ExecuteProcess>(options);

// 执行
MockLaunchContext context;
auto result = action->Execute(context);
if (result.HasError()) {
    std::cerr << "Failed: " << result.GetError().GetMessage() << std::endl;
}
```

---

### DeclareLaunchArgument

声明启动参数。

```cpp
class DeclareLaunchArgument : public Action {
public:
    struct Options {
        std::string name;
        std::string description;
        SubstitutionPtr defaultValue;
        std::vector<std::string> choices;
    };
    
    explicit DeclareLaunchArgument(const Options& options);
    
    Result<void> Execute(LaunchContext& context) override;
    
    // 获取参数信息
    const std::string& GetName() const;
    const std::string& GetDescription() const;
    std::string GetDefaultValue(const LaunchContext& context) const;
};
```

---

## 事件系统

### Event

事件基类。

```cpp
class Event {
public:
    virtual ~Event() = default;
    virtual std::string GetType() const = 0;
    std::chrono::steady_clock::time_point GetTimestamp() const;
};
```

### ProcessStartedEvent

进程启动事件。

```cpp
class ProcessStartedEvent : public Event {
public:
    ProcessStartedEvent(std::int32_t pid, const std::string& name);
    
    std::string GetType() const override { return "process_started"; }
    std::int32_t GetPid() const { return pid_; }
    const std::string& GetName() const { return name_; }
};
```

### EventHandler

事件处理器基类。

```cpp
class EventHandler {
public:
    virtual ~EventHandler() = default;
    virtual bool Matches(const Event& event) const = 0;
    virtual void Handle(const Event& event, LaunchContext& context) = 0;
};
```

---

## 条件与替换

### Condition

条件基类。

```cpp
class Condition {
public:
    virtual ~Condition() = default;
    virtual bool Evaluate(const LaunchContext& context) const = 0;
};
```

### IfCondition

真值条件。

```cpp
class IfCondition : public Condition {
public:
    explicit IfCondition(const SubstitutionPtr& expression);
    bool Evaluate(const LaunchContext& context) const override;
};
```

**使用示例**:

```cpp
auto condition = std::make_shared<IfCondition>(
    std::make_shared<TextSubstitution>("true")
);

MockLaunchContext context;
bool result = condition->Evaluate(context);  // true
```

---

### Substitution

替换基类。

```cpp
class Substitution {
public:
    virtual ~Substitution() = default;
    virtual std::string Perform(const LaunchContext& context) const = 0;
};
```

### TextSubstitution

文本替换。

```cpp
class TextSubstitution : public Substitution {
public:
    explicit TextSubstitution(const std::string& text);
    std::string Perform(const LaunchContext& context) const override;
};
```

### LaunchConfiguration

启动配置替换。

```cpp
class LaunchConfiguration : public Substitution {
public:
    explicit LaunchConfiguration(const std::string& name);
    std::string Perform(const LaunchContext& context) const override;
};
```

### EnvironmentVariable

环境变量替换。

```cpp
class EnvironmentVariable : public Substitution {
public:
    explicit EnvironmentVariable(const std::string& name);
    std::string Perform(const LaunchContext& context) const override;
};
```

### VariableSubstitution

变量替换 ($(var name))。

```cpp
class VariableSubstitution : public Substitution {
public:
    VariableSubstitution(const std::string& variable_name, 
                        const std::string& default_value = "");
    std::string Perform(const LaunchContext& context) const override;
};
```

---

## 安全架构

### ProcessExecutor

进程执行接口。

```cpp
class ProcessExecutor {
public:
    virtual ~ProcessExecutor() = default;
    
    virtual OsalResult<ProcessId> Execute(
        const CommandLine& command, 
        const ProcessOptions& options) = 0;
    
    virtual OsalResult<ProcessResult> Wait(
        ProcessId pid, 
        std::chrono::milliseconds timeout) = 0;
    
    virtual OsalResult<void> SendSignal(ProcessId pid, int signal) = 0;
    virtual OsalResult<void> Terminate(ProcessId pid) = 0;
};
```

### ResourceMonitor

资源监控接口。

```cpp
class ResourceMonitor {
public:
    virtual ~ResourceMonitor() = default;
    
    virtual OsalResult<bool> AreResourcesAvailable(uint64_t estimated_memory) = 0;
    virtual OsalResult<ResourceUsage> GetProcessResources(ProcessId pid) = 0;
    virtual OsalResult<void> SetResourceLimits(
        ProcessId pid, uint64_t max_memory, double max_cpu) = 0;
};
```

### Watchdog

看门狗接口。

```cpp
class Watchdog {
public:
    virtual ~Watchdog() = default;
    
    virtual OsalResult<void> Start() = 0;
    virtual void Stop() = 0;
    
    virtual OsalResult<void> RegisterNode(
        uint32_t node_id, 
        uint32_t timeout_ms,
        HeartbeatCallback callback) = 0;
    
    virtual OsalResult<void> UnregisterNode(uint32_t node_id) = 0;
};
```

---

## 依赖管理

### DependencyResolver

依赖解析器。

```cpp
class DependencyResolver {
public:
    ResolutionResult Resolve(const std::vector<NodeConfig>& nodes);
    bool HasCircularDependency(const std::vector<NodeConfig>& nodes);
    std::vector<std::string> GetCircularPath(const std::vector<NodeConfig>& nodes);
    bool ValidateDependencies(const std::vector<NodeConfig>& nodes);
};
```

### DependencyManager

依赖管理器。

```cpp
class DependencyManager {
public:
    Error AddProcess(const std::string& name,
                    const std::shared_ptr<ExecuteProcess>& action,
                    const std::vector<std::string>& dependencies);
    
    ResolutionResult ResolveDependencies() const;
    Error ExecuteAll(LaunchContext& context);
    
    std::shared_ptr<ExecuteProcess> GetProcess(const std::string& name) const;
    bool IsReady(const std::string& name, const std::set<std::string>& completed) const;
    
    void Clear();
    size_t GetProcessCount() const;
};
```

---

## 使用示例

### 完整示例

```cpp
#include "launch_cpp/launch_service.hpp"
#include "launch_cpp/launch_description.hpp"
#include "launch_cpp/actions/execute_process.hpp"
#include "launch_cpp/actions/declare_launch_argument.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"
#include "launch_cpp/substitutions/variable_substitution.hpp"

using namespace launch_cpp;

int main() {
    // 创建服务
    LaunchService service;
    
    // 创建启动描述
    auto desc = std::make_shared<LaunchDescription>();
    
    // 声明参数
    DeclareLaunchArgument::Options arg_opts;
    arg_opts.name = "message";
    arg_opts.defaultValue = std::make_shared<TextSubstitution>("Hello");
    desc->Add(std::make_shared<DeclareLaunchArgument>(arg_opts));
    
    // 执行进程
    ExecuteProcess::Options proc_opts;
    proc_opts.cmd = {
        std::make_shared<TextSubstitution>("echo"),
        std::make_shared<VariableSubstitution>("message")
    };
    proc_opts.output = "screen";
    desc->Add(std::make_shared<ExecuteProcess>(proc_opts));
    
    // 包含描述
    auto error = service.IncludeLaunchDescription(desc);
    if (error.IsError()) {
        std::cerr << "Error: " << error.GetMessage() << std::endl;
        return 1;
    }
    
    // 运行
    return service.Run();
}
```

---

## 错误处理

### Error 类

```cpp
class Error {
public:
    Error();  // 默认构造，表示无错误
    Error(ErrorCode code, const std::string& message);
    
    bool IsError() const;
    ErrorCode GetCode() const;
    const std::string& GetMessage() const;
};
```

### ErrorCode 枚举

```cpp
enum class ErrorCode : std::int32_t {
    kSuccess = 0,
    kInvalidArgument = 1,
    kInvalidConfiguration = 2,
    kProcessSpawnFailed = 3,
    kProcessNotFound = 4,
    kEventHandlerError = 5,
    kSubstitutionError = 6,
    kCyclicDependency = 7,
    kTimeout = 8,
    kShutdownRequested = 9,
    kNotImplemented = 10,
    kInternalError = 11,
    kResourceExhausted = 12,
    kMaxRetriesExceeded = 13,
    kUnknownError = 99
};
```

### Result<T> 模板

```cpp
template<typename T>
class Result {
public:
    Result();  // 默认构造成功
    explicit Result(const T& value);
    explicit Result(const Error& error);
    
    bool IsSuccess() const;
    bool HasError() const;
    bool HasValue() const;
    
    const Error& GetError() const;
    T& GetValue();
    const T& GetValue() const;
};
```