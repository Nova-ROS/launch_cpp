# ROS2 Package 目录结构

## cpp_launch/ (主包)

```
cpp_launch/
├── package.xml                  # ROS2 package 清单
├── CMakeLists.txt               # 构建配置
├── README.md                    # 包说明文档
│
├── include/cpp_launch/          # 公共头文件
│   ├── types.hpp               # 基础类型定义
│   ├── error_code.hpp          # 错误码系统
│   ├── singleton.hpp           # 单例模式
│   ├── thread_pool.hpp         # 线程池
│   │
│   ├── launch_service.hpp      # LaunchService 接口
│   ├── launch_context.hpp      # LaunchContext 接口
│   ├── launch_description.hpp  # LaunchDescription
│   ├── launch_description_entity.hpp
│   │
│   ├── event.hpp               # Event 基类
│   ├── event_handler.hpp       # EventHandler 接口
│   ├── substitution.hpp        # Substitution 接口
│   ├── condition.hpp           # Condition 接口
│   ├── action.hpp              # Action 基类
│   │
│   ├── actions/                # 动作实现
│   │   ├── execute_process.hpp
│   │   ├── declare_launch_argument.hpp
│   │   ├── include_launch_description.hpp
│   │   ├── set_launch_configuration.hpp
│   │   ├── register_event_handler.hpp
│   │   ├── emit_event.hpp
│   │   ├── timer_action.hpp
│   │   └── group_action.hpp
│   │
│   ├── substitutions/          # 替换实现
│   │   ├── text_substitution.hpp
│   │   ├── launch_configuration.hpp
│   │   ├── environment_variable.hpp
│   │   ├── find_executable.hpp
│   │   ├── command.hpp
│   │   └── this_launch_file.hpp
│   │
│   └── conditions/             # 条件实现
│       ├── if_condition.hpp
│       ├── unless_condition.hpp
│       └── launch_configuration_equals.hpp
│
├── src/                        # 实现文件
│   ├── launch_service.cpp
│   ├── launch_context.cpp
│   ├── launch_description.cpp
│   ├── event.cpp
│   ├── event_handler.cpp
│   ├── event_queue.cpp
│   ├── event_dispatcher.cpp
│   ├── substitution.cpp
│   ├── condition.cpp
│   ├── action.cpp
│   ├── thread_pool.cpp
│   ├── async_pipe.cpp
│   ├── process.cpp
│   ├── process_posix.cpp
│   ├── signal_handler.cpp
│   │
│   ├── actions/                # 动作实现
│   │   ├── execute_process.cpp
│   │   ├── declare_launch_argument.cpp
│   │   ├── include_launch_description.cpp
│   │   ├── set_launch_configuration.cpp
│   │   ├── register_event_handler.cpp
│   │   ├── emit_event.cpp
│   │   ├── timer_action.cpp
│   │   └── group_action.cpp
│   │
│   ├── substitutions/          # 替换实现
│   │   ├── text_substitution.cpp
│   │   ├── launch_configuration.cpp
│   │   ├── environment_variable.cpp
│   │   ├── find_executable.cpp
│   │   ├── command.cpp
│   │   └── this_launch_file.cpp
│   │
│   └── conditions/             # 条件实现
│       ├── if_condition.cpp
│       ├── unless_condition.cpp
│       └── launch_configuration_equals.cpp
│
├── test/                       # 测试文件
│   ├── test_launch_context.cpp
│   ├── test_substitution.cpp
│   ├── test_process.cpp
│   ├── test_launch_service.cpp
│   ├── test_actions.cpp
│   └── test_yaml_parser.cpp
│
├── examples/                   # 示例文件
│   ├── example_basic.cpp
│   ├── example_ros_node.cpp
│   ├── example_events.cpp
│   └── example_yaml.launch.yaml
│
└── cmake/                      # CMake 模块
    └── cpp_launch-config.cmake.in
```

## cpp_launch_ros/ (ROS 扩展包)

```
cpp_launch_ros/
├── package.xml
├── CMakeLists.txt
├── README.md
│
├── include/cpp_launch_ros/
│   ├── node.hpp               # ROS Node Action
│   ├── lifecycle_node.hpp     # Lifecycle Node
│   ├── composable_node_container.hpp
│   ├── load_composable_nodes.hpp
│   ├── set_parameter.hpp
│   ├── push_ros_namespace.hpp
│   │
│   └── substitutions/
│       ├── find_package.hpp
│       ├── executable_in_package.hpp
│       └── find_package_share.hpp
│
├── src/
│   ├── node.cpp
│   ├── lifecycle_node.cpp
│   ├── composable_node_container.cpp
│   ├── load_composable_nodes.cpp
│   ├── set_parameter.cpp
│   ├── push_ros_namespace.cpp
│   │
│   └── substitutions/
│       ├── find_package.cpp
│       ├── executable_in_package.cpp
│       └── find_package_share.cpp
│
└── test/
    ├── test_node.cpp
    └── test_lifecycle_node.cpp
```

## cpp_launch_testing/ (测试工具包)

```
cpp_launch_testing/
├── package.xml
├── CMakeLists.txt
├── README.md
│
├── include/cpp_launch_testing/
│   ├── launch_test.hpp
│   ├── process_test.hpp
│   └── mock_context.hpp
│
└── src/
    └── test_main.cpp
```

## 构建输出结构

```
build/
├── cpp_launch/
│   ├── CMakeFiles/
│   ├── libcpp_launch_core.so
│   ├── libcpp_launch_actions.so
│   ├── libcpp_launch_substitutions.so
│   ├── libcpp_launch_conditions.so
│   └── cpp_launch (executable)
│
├── cpp_launch_ros/
│   ├── CMakeFiles/
│   └── libcpp_launch_ros.so
│
└── cpp_launch_testing/
    └── test_*

install/
├── include/
│   ├── cpp_launch/
│   └── cpp_launch_ros/
├── lib/
│   ├── libcpp_launch*.so
│   └── cmake/cpp_launch/
├── bin/
│   └── cpp_launch
└── share/
    ├── cpp_launch/
    │   ├── package.xml
    │   └── cmake/
    └── cpp_launch_ros/
        └── package.xml
```

## AUTOSAR C++14 合规性文件

### 编码规范检查
- `.ament_blacklist`: 禁用某些 lint 规则
- `.autosar-config`: AUTOSAR 检查配置

### 关键合规点

1. **A15-0-1**: 不使用异常 - 使用 Error/Result 替代
2. **A18-5-2**: 不使用 std::exception - 自定义错误处理
3. **A7-2-4**: 使用 enum class - 所有枚举类型
4. **A12-8-4**: 虚析构函数 - 所有基类
5. **M0-1-9**: noexcept 标记 - 不抛出异常的函数
6. **A10-3-3**: 特殊函数声明 - 所有类明确声明
