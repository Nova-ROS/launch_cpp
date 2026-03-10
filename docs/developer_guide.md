# launch_cpp 开发者指南

## 1. 开发环境设置

### 1.1 系统要求

- **操作系统**: Linux (Ubuntu 20.04+ 推荐)
- **编译器**: GCC 7+ 或 Clang 5+
- **CMake**: 3.14+
- **C++ 标准**: C++14

### 1.2 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    python3-colcon-common-extensions

# 可选：安装开发工具
sudo apt-get install -y \
    clang-format \
    cppcheck \
    valgrind
```

### 1.3 获取代码

```bash
cd ~/work/ros2/jazzy/src/ros2
git clone <repository-url> launch_cpp
cd launch_cpp
```

### 1.4 构建项目

```bash
# 使用 build.sh 脚本
cd ~/work/ros2/jazzy
./build.sh

# 或手动构建
cd src/ros2/launch_cpp
mkdir -p build && cd build
cmake .. -DBUILD_TESTING=ON
make -j$(nproc)
```

---

## 2. 项目结构

```
launch_cpp/
├── CMakeLists.txt              # 构建配置
├── package.xml                 # ROS2 包配置
├── README.md                   # 项目说明
├── LICENSE                     # Apache 2.0 许可证
│
├── include/launch_cpp/         # 公共头文件
│   ├── launch_service.hpp      # 主服务
│   ├── launch_context.hpp      # 运行时上下文
│   ├── launch_description.hpp  # 启动描述
│   ├── action.hpp              # 动作基类
│   ├── event.hpp               # 事件系统
│   ├── substitution.hpp        # 替换基类
│   ├── condition.hpp           # 条件基类
│   ├── error_code.hpp          # 错误处理
│   ├── types.hpp               # 基本类型
│   ├── dependency_manager.hpp  # 依赖管理
│   ├── actions/                # 动作实现头文件
│   ├── substitutions/          # 替换实现头文件
│   ├── conditions/             # 条件实现头文件
│   └── safety/                 # 安全架构
│
├── src/                        # 实现文件
│   ├── launch_service.cpp
│   ├── launch_description.cpp
│   ├── yaml_parser.cpp         # YAML 解析器
│   ├── thread_pool.cpp         # 线程池
│   ├── action.cpp
│   ├── dependency_manager.cpp
│   ├── actions/                # 动作实现
│   ├── substitutions/          # 替换实现
│   ├── conditions/             # 条件实现
│   └── safety/                 # 安全实现
│
├── test/                       # 测试文件
│   ├── test_*.cpp              # 各功能测试
│   └── CMakeLists.txt
│
├── examples/                   # 示例文件
│   ├── example_launch.yaml
│   ├── test_simple.yaml
│   └── example_with_dependencies.yaml
│
└── docs/                       # 文档
    ├── architecture.md         # 架构文档
    ├── functional_spec.md      # 功能规格
    ├── api.md                  # API 文档
    ├── developer_guide.md      # 本文件
    └── ZERO_DEPENDENCIES.md    # 零依赖说明
```

---

## 3. 代码规范

### 3.1 命名规范

| 类型 | 规范 | 示例 |
|------|------|------|
| **类名** | PascalCase | `LaunchService`, `ExecuteProcess` |
| **函数** | snake_case | `run()`, `execute()` |
| **变量** | snake_case | `options`, `context` |
| **常量** | UPPER_SNAKE_CASE | `MAX_RETRIES` |
| **宏** | UPPER_SNAKE_CASE | `LAUNCH_CPP__...` |
| **命名空间** | snake_case | `launch_cpp` |
| **成员变量** | snake_case + 后缀_ | `options_`, `context_` |
| **模板参数** | PascalCase | `T`, `InputIt` |

### 3.2 文件组织

- **头文件**: `.hpp`
- **实现文件**: `.cpp`
- **文件名**: snake_case
- **头文件保护**: `LAUNCH_CPP__<PATH>__HPP_`

示例:
```cpp
// include/launch_cpp/actions/execute_process.hpp
#ifndef LAUNCH_CPP__ACTIONS__EXECUTE_PROCESS_HPP_
#define LAUNCH_CPP__ACTIONS__EXECUTE_PROCESS_HPP_

// ... content ...

#endif // LAUNCH_CPP__ACTIONS__EXECUTE_PROCESS_HPP_
```

### 3.3 代码格式

- **缩进**: 2 个空格
- **行宽**: 100 字符
- **括号**: 同行
- **对齐**: 左对齐

示例:
```cpp
class MyClass {
 public:
  explicit MyClass(const std::string& name);
  ~MyClass();
  
  void DoSomething();
  
 private:
  std::string name_;
};
```

### 3.4 AUTOSAR C++14 规范

#### 禁止项
- ❌ 异常 (exceptions)
- ❌ RTTI (dynamic_cast, typeid)
- ❌ 标准库异常类
- ❌ 裸 new/delete (使用智能指针)
- ❌ 隐式转换

#### 必须项
- ✅ 使用 `Result<T>` 错误处理
- ✅ 虚析构函数
- ✅ 显式构造函数
- ✅ `override` 关键字
- ✅ `enum class`

---

## 4. 开发工作流

### 4.1 添加新功能

#### 步骤 1: 设计

1. 确定功能范围
2. 设计接口
3. 编写测试用例

#### 步骤 2: 实现

1. 创建头文件 (`include/launch_cpp/...`)
2. 创建实现文件 (`src/...`)
3. 添加单元测试 (`test/test_*.cpp`)

#### 步骤 3: 集成

1. 更新 `CMakeLists.txt`
2. 更新文档
3. 运行所有测试

### 4.2 示例：添加新 Action

**1. 创建头文件**

```cpp
// include/launch_cpp/actions/my_action.hpp
#ifndef LAUNCH_CPP__ACTIONS__MY_ACTION_HPP_
#define LAUNCH_CPP__ACTIONS__MY_ACTION_HPP_

#include "launch_cpp/action.hpp"

namespace launch_cpp {

class MyAction : public Action {
 public:
  struct Options {
    std::string name;
    int value = 0;
  };
  
  explicit MyAction(const Options& options);
  ~MyAction() override = default;
  
  Result<void> Execute(LaunchContext& context) override;
  
 private:
  Options options_;
};

}  // namespace launch_cpp

#endif
```

**2. 创建实现文件**

```cpp
// src/actions/my_action.cpp
#include "launch_cpp/actions/my_action.hpp"
#include "launch_cpp/launch_context.hpp"

namespace launch_cpp {

MyAction::MyAction(const Options& options)
    : options_(options) {}

Result<void> MyAction::Execute(LaunchContext& context) {
  // 实现逻辑
  (void)context;
  
  // 返回成功
  return Result<void>();
  
  // 或返回错误
  // return Result<void>(Error(ErrorCode::kInvalidArgument, "..."));
}

}  // namespace launch_cpp
```

**3. 添加测试**

```cpp
// test/test_my_action.cpp
#include <gtest/gtest.h>
#include "launch_cpp/actions/my_action.hpp"

using namespace launch_cpp;

TEST(MyActionTest, BasicExecution) {
  MyAction::Options options;
  options.name = "test";
  options.value = 42;
  
  auto action = std::make_shared<MyAction>(options);
  
  MockLaunchContext context;
  auto result = action->Execute(context);
  
  EXPECT_TRUE(result.IsSuccess());
}
```

**4. 更新 CMakeLists.txt**

```cmake
# 添加到 CPP_LAUNCH_CORE_SOURCES
src/actions/my_action.cpp
```

---

## 5. 测试

### 5.1 运行测试

```bash
# 所有测试
cd build/launch_cpp
ctest -V

# 特定测试
./test_yaml_parser
./test_actions_comprehensive

# 带过滤器
./test_actions_comprehensive --gtest_filter=ExecuteProcessTest.*
```

### 5.2 测试规范

- **单元测试**: 每个类一个测试文件
- **命名**: `test_<feature>.cpp`
- **覆盖率**: 目标 85%+
- **Mock**: 使用 Mock 对象隔离测试

### 5.3 测试模板

```cpp
#include <gtest/gtest.h>
#include "launch_cpp/..."

using namespace launch_cpp;

// 测试固件
class MyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 初始化
  }
  
  void TearDown() override {
    // 清理
  }
};

// 基本测试
TEST(MyTest, BasicCase) {
  // Arrange
  auto obj = std::make_shared<MyClass>();
  
  // Act
  auto result = obj->DoSomething();
  
  // Assert
  EXPECT_TRUE(result.IsSuccess());
  EXPECT_EQ(result.GetValue(), expected);
}

// 错误测试
TEST(MyTest, ErrorCase) {
  auto result = DoSomethingInvalid();
  
  EXPECT_TRUE(result.HasError());
  EXPECT_EQ(result.GetError().GetCode(), ErrorCode::kInvalidArgument);
}
```

---

## 6. 调试技巧

### 6.1 使用 GDB

```bash
# 编译调试版本
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 启动 GDB
gdb ./test_yaml_parser

# 常用命令
(gdb) break main
(gdb) run
(gdb) next
(gdb) print variable
(gdb) backtrace
```

### 6.2 内存检查

```bash
# Valgrind
valgrind --leak-check=full ./test_yaml_parser

# AddressSanitizer
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address"
make
./test_yaml_parser
```

### 6.3 日志输出

```cpp
// 临时调试输出
std::cerr << "[DEBUG] Variable value: " << value << std::endl;
```

---

## 7. 性能优化

### 7.1 性能测试

```bash
# 使用 Google Benchmark
./benchmark_launch
```

### 7.2 优化建议

1. **减少拷贝**: 使用 `const&` 和移动语义
2. **智能指针**: 使用 `shared_ptr` 和 `unique_ptr`
3. **预分配**: 预先分配 vector 容量
4. **避免虚函数**: 在热路径中使用模板

### 7.3 性能分析

```bash
# perf
perf record ./test_yaml_parser
perf report

# gprof
cmake .. -DCMAKE_CXX_FLAGS="-pg"
make
./test_yaml_parser
gprof ./test_yaml_parser gmon.out
```

---

## 8. 文档规范

### 8.1 代码注释

**文件头**:
```cpp
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
```

**函数注释** (Doxygen 格式):
```cpp
/**
 * @brief 简短描述
 * @details 详细描述
 * @param param_name 参数说明
 * @return 返回值说明
 * @pre 前置条件
 * @post 后置条件
 * @note 注意事项
 * @thread_safety 线程安全说明
 * @requirements REQ-XXX-XXX
 */
```

### 8.2 文档维护

- **架构变更**: 更新 `architecture.md`
- **API 变更**: 更新 `api.md`
- **功能变更**: 更新 `functional_spec.md`

---

## 9. 提交规范

### 9.1 提交信息格式

```
<type>: <subject>

<body>

<footer>
```

**Type**:
- `feat`: 新功能
- `fix`: 修复
- `docs`: 文档
- `style`: 格式
- `refactor`: 重构
- `test`: 测试
- `chore`: 构建/工具

**示例**:
```
feat: add VariableSubstitution support

- Implement $(var name) syntax
- Add VariableSubstitution class
- Update YAML parser to detect variable syntax
- Add 7 unit tests

Closes #123
```

### 9.2 代码审查检查清单

- [ ] 代码符合规范
- [ ] 所有测试通过
- [ ] 新增代码有测试
- [ ] 文档已更新
- [ ] 无内存泄漏
- [ ] 性能无退化

---

## 10. 常见问题

### Q1: 编译错误 "undefined reference"

**原因**: 未链接库或缺少实现

**解决**:
```bash
# 清理重新构建
rm -rf build
mkdir build && cd build
cmake .. -DBUILD_TESTING=ON
make
```

### Q2: 测试失败 "Result has error"

**调试**:
```cpp
auto result = action.Execute(context);
if (result.HasError()) {
  std::cerr << "Error: " << result.GetError().GetMessage() << std::endl;
}
```

### Q3: YAML 解析失败

**检查**:
- YAML 格式正确
- 缩进使用空格
- 特殊字符转义

### Q4: 进程无法启动

**检查**:
- 命令存在且可执行
- 路径正确
- 权限足够

---

## 11. 资源链接

- **AUTOSAR C++14**: https://www.autosar.org/
- **ISO 26262**: https://www.iso.org/
- **ROS2 Launch**: https://docs.ros.org/
- **CMake**: https://cmake.org/documentation/
- **Google Test**: https://google.github.io/googletest/

---

## 12. 联系与支持

- **Issue 跟踪**: GitHub Issues
- **邮件列表**: launch_cpp@example.com
- **文档**: See `docs/` directory

---

**Happy Coding! 🚀**