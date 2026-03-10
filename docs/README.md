# launch_cpp 文档中心

欢迎使用 launch_cpp 文档！这里包含了项目的完整文档集。

## 📚 文档列表

### 🏗️ 架构与设计

- **[架构文档](architecture.md)** - 系统架构、组件设计、技术选型
  - 五层架构设计
  - 安全架构 (ISO 26262)
  - 设计模式
  - 线程模型
  - 数据流

- **[功能规格说明书](functional_spec.md)** - 详细功能规格
  - 功能范围
  - 接口定义
  - 执行流程
  - 性能指标
  - 兼容性说明

### 💻 API 参考

- **[API 文档](api.md)** - 完整 API 参考
  - 核心类 (LaunchService, LaunchContext)
  - 动作类 (ExecuteProcess, etc.)
  - 事件系统
  - 条件与替换
  - 安全架构
  - 使用示例

### 🛠️ 开发指南

- **[开发者指南](developer_guide.md)** - 开发指南
  - 环境设置
  - 项目结构
  - 代码规范
  - 开发工作流
  - 测试规范
  - 调试技巧
  - 提交规范

### 📖 其他文档

- **[README](../README.md)** - 项目概览和快速开始
- **[ZERO_DEPENDENCIES.md](ZERO_DEPENDENCIES.md)** - 零依赖实现说明
- **[DIRECTORY_STRUCTURE.md](../DIRECTORY_STRUCTURE.md)** - 目录结构说明

---

## 🎯 快速导航

### 如果你是...

**新用户** → 从 [README](../README.md) 开始

**架构师** → 阅读 [架构文档](architecture.md)

**开发者** → 查看 [开发者指南](developer_guide.md)

**API 使用者** → 参考 [API 文档](api.md)

**测试人员** → 查看 [功能规格说明书](functional_spec.md)

---

## 🚀 快速开始

```bash
# 构建
cd ~/work/ros2/jazzy
./build.sh

# 运行示例
./install/launch_cpp/bin/launch_cpp \
  ./install/launch_cpp/share/launch_cpp/examples/test_simple.yaml

# 运行测试
cd build/launch_cpp
ctest -V
```

---

## 📂 文档结构

```
docs/
├── README.md                 # 本文件
├── architecture.md           # 架构文档
├── functional_spec.md        # 功能规格
├── api.md                    # API 文档
├── developer_guide.md        # 开发者指南
└── ZERO_DEPENDENCIES.md      # 零依赖说明
```

---

## 📝 文档规范

- 所有文档使用 Markdown 格式
- 代码示例使用 ```cpp 标记
- 图表使用 ASCII 或文字描述
- 保持文档与代码同步更新

---

## 🤝 贡献

如果发现文档问题或需要改进：

1. 提交 Issue 描述问题
2. 或提交 Pull Request 更新文档

---

## 📞 支持

- **Issue 跟踪**: GitHub Issues
- **文档反馈**: 通过 Issue 或 PR

---

**最后更新**: 2026年3月10日

**版本**: launch_cpp 0.1.0