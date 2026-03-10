# launch_cpp 功能规格说明书

## 1. 功能概述

### 1.1 功能范围

launch_cpp 提供以下核心功能：

| 功能类别 | 功能项 | 状态 |
|---------|--------|------|
| **核心功能** | 进程执行 (ExecuteProcess) | ✅ |
| | 启动参数声明 (DeclareLaunchArgument) | ✅ |
| | 启动文件包含 (IncludeLaunchDescription) | ✅ |
| | 配置设置 (SetLaunchConfiguration) | ✅ |
| | 定时器 (TimerAction) | ✅ |
| | 分组 (GroupAction) | ✅ |
| **条件系统** | IfCondition | ✅ |
| | UnlessCondition | ✅ |
| | LaunchConfigurationEquals | ✅ |
| **替换系统** | TextSubstitution | ✅ |
| | LaunchConfiguration | ✅ |
| | EnvironmentVariable | ✅ |
| | VariableSubstitution ($(var)) | ✅ |
| **依赖管理** | DependencyResolver | ✅ |
| | DependencyManager | ✅ |
| **安全功能** | ResourceMonitor | ✅ |
| | Watchdog | ✅ |
| | RetryPolicy | ✅ |
| **解析器** | YAML 解析 | ✅ |

### 1.2 功能特性

#### 1.2.1 进程管理

**功能描述**: 启动、监控和管理外部进程

**输入**:
- 命令行参数 (cmd)
- 工作目录 (cwd)
- 环境变量 (env)
- 输出模式 (output: screen/log/both)

**输出**:
- 进程 PID
- 进程状态 (运行/停止)
- 退出码

**特性**:
- 支持 TTY 模拟
- SIGTERM/SIGKILL 关闭
- 超时控制
- 资源限制 (内存/CPU)

#### 1.2.2 依赖管理

**功能描述**: 管理进程间的依赖关系和启动顺序

**输入**:
- 进程列表 (带名称)
- 依赖关系 (depends_on)

**输出**:
- 拓扑排序后的启动顺序
- 循环依赖检测
- 缺失依赖检测

**算法**: Kahn's Algorithm (O(V+E))

#### 1.2.3 变量替换

**功能描述**: 支持动态值替换

**支持语法**:
```yaml
$(var variable_name)          # 启动配置变量
$(env ENV_VAR)                # 环境变量
$(find package_name)          # 查找包路径
```

**解析时机**: 运行时动态解析

## 2. 功能详细规格

### 2.1 ExecuteProcess

#### 2.1.1 接口定义

```cpp
struct Options {
    // 命令行
    std::vector<SubstitutionPtr> cmd;
    
    // 工作目录
    SubstitutionPtr cwd;
    
    // 环境变量
    std::unordered_map<std::string, SubstitutionPtr> env;
    
    // 输出模式
    std::string output = "log";  // "screen", "log", "both"
    
    // TTY 模拟
    bool emulateTty = false;
    
    // 关闭超时
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
```

#### 2.1.2 执行流程

```
1. 解析命令行
   └── 执行所有 Substitution → 实际参数列表
   
2. 检查依赖
   └── 依赖管理器检查 dependsOn
   
3. 资源检查 (如果启用安全)
   └── ResourceMonitor::AreResourcesAvailable()
   
4. 执行进程
   ├── fork() 创建子进程
   ├── execvp() 执行命令
   └── 父进程记录 PID
   
5. 注册监控 (如果启用安全)
   ├── Watchdog::RegisterNode()
   └── ResourceMonitor::SetResourceLimits()
   
6. 监控状态
   └── 等待进程退出或发送事件
```

#### 2.1.3 错误处理

| 错误码 | 描述 | 处理方式 |
|--------|------|----------|
| kProcessSpawnFailed | 进程创建失败 | 返回错误，可选择重试 |
| kResourceExhausted | 资源不足 | 返回错误 |
| kTimeout | 超时 | 发送 SIGTERM，可选择重试 |
| kMaxRetriesExceeded | 重试次数用尽 | 返回错误 |

### 2.2 DependencyResolver

#### 2.2.1 算法规格

**算法**: Kahn's Algorithm (拓扑排序)

**输入**:
```cpp
struct NodeConfig {
    std::string name;
    std::vector<std::string> dependencies;
};
```

**输出**:
```cpp
struct ResolutionResult {
    bool success;
    std::vector<std::string> order;
    std::string error_message;
    std::vector<std::string> circular_path;
};
```

**复杂度**:
- 时间: O(V + E)
- 空间: O(V)

#### 2.2.2 验证功能

1. **缺失依赖检测**
   - 检查所有依赖是否存在于节点列表中
   - 返回缺失的依赖名称列表

2. **循环依赖检测**
   - 使用 DFS 检测有向图中的环
   - 返回循环路径

3. **就绪节点查询**
   - 给定已启动节点集合
   - 返回可以启动的节点列表

### 2.3 VariableSubstitution

#### 2.3.1 语法规格

```
$(var <variable_name>)
$(env <environment_variable>)
$(find <package_name>)
```

#### 2.3.2 解析规则

1. **变量查找顺序**:
   - Launch Configuration (启动配置)
   - Environment Variable (环境变量)
   - Default Value (默认值)

2. **未找到处理**:
   - 如果有默认值，返回默认值
   - 否则返回空字符串

3. **嵌套支持**:
   - 当前不支持嵌套替换
   - 未来可扩展: $(var $(var inner))

### 2.4 YAML 解析器

#### 2.4.1 支持的 YAML 结构

```yaml
# 基本结构
entities:
  - type: execute_process
    name: process_name
    cmd:
      - command
      - arg1
      - arg2
    output: screen
    depends_on:
      - other_process

# 参数声明
  - type: declare_launch_argument
    name: arg_name
    default_value: default
    description: "Argument description"

# 配置设置
  - type: set_launch_configuration
    name: config_name
    value: config_value
```

#### 2.4.2 类型映射

| YAML 类型 | C++ 类型 |
|-----------|----------|
| string | std::string |
| integer | std::int32_t |
| float | double |
| boolean | bool |
| array | std::vector |
| object | std::map |

### 2.5 安全功能

#### 2.5.1 ResourceMonitor

**功能**:
- 检查系统资源可用性
- 设置进程资源限制
- 监控资源使用

**接口**:
```cpp
class ResourceMonitor {
public:
    virtual OsalResult<bool> AreResourcesAvailable(uint64_t estimated_memory) = 0;
    virtual OsalResult<void> SetResourceLimits(ProcessId pid, uint64_t max_memory, double max_cpu) = 0;
    virtual OsalResult<SystemResources> GetSystemResources() = 0;
};
```

#### 2.5.2 Watchdog

**功能**:
- 注册进程监控
- 心跳检测
- 超时处理

**机制**:
- 定期 ping 检测
- 超时自动重启或报警

#### 2.5.3 RetryPolicy

**功能**:
- 失败自动重试
- 指数退避
- 最大重试次数限制

**退避算法**:
```
delay = base_delay * (multiplier ^ attempt)
```

## 3. 功能使用示例

### 3.1 基本进程启动

```yaml
entities:
  - type: execute_process
    cmd:
      - echo
      - "Hello, World!"
    output: screen
```

### 3.2 带依赖的进程

```yaml
entities:
  - type: execute_process
    name: database
    cmd: [echo, "DB started"]
    
  - type: execute_process
    name: api
    cmd: [echo, "API started"]
    depends_on:
      - database
```

### 3.3 变量替换

```yaml
entities:
  - type: declare_launch_argument
    name: message
    default_value: "Hello"
  
  - type: execute_process
    cmd:
      - echo
      - $(var message)
```

### 3.4 安全配置

```yaml
entities:
  - type: execute_process
    cmd: [ros2, run, my_pkg, my_node]
    enableSafety: true
    maxMemoryBytes: 104857600  # 100MB
    maxCpuPercent: 50.0
    watchdogTimeoutMs: 1000
    maxRetries: 3
```

## 4. 功能限制

### 4.1 当前限制

1. **YAML 解析**
   - 不支持 YAML 锚点和别名
   - 不支持多行字符串 (| 和 >)
   - 不支持二进制数据

2. **替换系统**
   - 不支持嵌套替换
   - 不支持算术表达式

3. **进程管理**
   - 仅支持 POSIX 系统
   - 不支持 Windows 进程管理

4. **ROS2 集成**
   - 需要手动集成 ROS2 客户端库
   - 不支持 ROS1 桥接

### 4.2 未来扩展

1. **XML 支持**: 解析 XML 格式启动文件
2. **Python DSL**: 支持 Python 风格的 DSL
3. **远程执行**: 支持 SSH 远程启动进程
4. **容器支持**: Docker/Podman 集成
5. **Web UI**: 图形化监控界面

## 5. 性能规格

### 5.1 性能指标

| 操作 | 时间 | 内存 |
|------|------|------|
| 启动文件解析 | <10ms | <100KB |
| 进程创建 | ~1ms | ~50KB |
| 依赖解析 (100 节点) | <5ms | ~100KB |
| 变量替换 | <1ms | 可忽略 |
| 事件处理 | <1ms | 可忽略 |

### 5.2 可扩展性

- **最大进程数**: 受系统限制 (通常 1000+)
- **最大依赖深度**: 无限制 (但建议 <10)
- **最大启动文件大小**: 10MB (YAML 解析限制)

## 6. 兼容性

### 6.1 ROS2 兼容性

- **语义兼容**: API 语义与 Python launch 相同
- **YAML 兼容**: 支持 ROS2 launch YAML 子集
- **事件兼容**: 事件类型和语义一致

### 6.2 平台兼容性

| 平台 | 支持 | 测试状态 |
|------|------|----------|
| Linux (x86_64) | ✅ | 完整测试 |
| Linux (ARM64) | ✅ | 未测试 |
| macOS | ⚠️ | 理论支持 |
| Windows | ❌ | 不支持 |

## 7. 安全考虑

### 7.1 命令注入防护

- **参数转义**: CommandBuilder::EscapeArgument()
- **验证**: 所有参数非空检查
- **白名单**: 仅允许安全字符

### 7.2 资源限制

- **内存限制**: 防止内存泄漏
- **CPU 限制**: 防止 CPU 耗尽
- **进程限制**: 防止 fork 炸弹

### 7.3 错误隔离

- **进程隔离**: 子进程崩溃不影响父进程
- **资源清理**: RAII 确保资源释放
- **超时机制**: 防止无限等待

## 8. 测试覆盖

### 8.1 功能测试

| 功能 | 测试数 | 覆盖率 |
|------|--------|--------|
| ExecuteProcess | 15 | 85% |
| DependencyResolver | 11 | 95% |
| VariableSubstitution | 7 | 100% |
| YAML 解析 | 26 | 90% |
| 条件系统 | 36 | 88% |
| 事件系统 | 19 | 82% |

### 8.2 集成测试

- 完整启动流程
- 多进程协调
- 错误恢复
- 安全功能

### 8.3 性能测试

- 大规模启动 (100+ 进程)
- 高频率事件处理
- 内存泄漏检测