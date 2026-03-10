# launch_cpp - Modern C++ Implementation of ROS2 Launch

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-14-blue.svg)](https://en.cppreference.com/w/cpp/14)
[![AUTOSAR](https://img.shields.io/badge/AUTOSAR-C%2B%2B14%20Compliant-green.svg)]()
[![ISO 26262](https://img.shields.io/badge/ISO%2026262-ASIL%20B-orange.svg)]()

## Overview

**launch_cpp** is a modern C++ implementation of the ROS2 launch system, fully compliant with **AUTOSAR C++14** standards and designed for **ISO 26262 ASIL B** functional safety requirements.

### Key Features

- **🚀 High Performance**: Compiled C++14, minimal runtime overhead
- **🔒 Type Safe**: Template-based `Result<T>` error handling, no exceptions
- **🛡️ Safety Compliant**: ISO 26262 ASIL B ready with comprehensive documentation
- **📦 Zero Dependencies**: Self-contained, uses only C++ standard library
- **🔧 Modular Design**: Clean separation with pluggable components
- **📝 YAML Support**: Native YAML launch file parsing
- **🔀 Dependency Management**: Automatic dependency resolution and startup ordering
- **🔄 Variable Substitution**: Support for `$(var name)` and `$(env VAR)` syntax

## Quick Start

### Prerequisites

- CMake >= 3.14
- C++14 compiler (GCC 7+ or Clang 5+)
- ROS2 Jazzy (optional, for ROS2 integration)

### Build

```bash
cd src/ros2/launch_cpp
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Run Examples

```bash
# Basic example
./install/launch_cpp/bin/launch_cpp \
  ./install/launch_cpp/share/launch_cpp/examples/test_simple.yaml

# With dependencies
./install/launch_cpp/bin/launch_cpp \
  ./install/launch_cpp/share/launch_cpp/examples/example_with_dependencies.yaml

# Programmatic usage
./install/launch_cpp/lib/launch_cpp/examples/example_basic
```

### Use as Library

```cpp
#include "launch_cpp/launch_service.hpp"
#include "launch_cpp/launch_description.hpp"
#include "launch_cpp/actions/execute_process.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"

using namespace launch_cpp;

int main()
{
  LaunchService service;
  auto desc = std::make_shared<LaunchDescription>();

  ExecuteProcess::Options options;
  options.cmd.push_back(std::make_shared<TextSubstitution>("echo"));
  options.cmd.push_back(std::make_shared<TextSubstitution>("Hello!"));
  options.output = "screen";

  desc->Add(std::make_shared<ExecuteProcess>(options));

  service.IncludeLaunchDescription(desc);
  return service.Run();
}
```

## YAML Launch Files

### Basic Example

```yaml
entities:
  - type: execute_process
    cmd:
      - echo
      - "Hello from launch_cpp!"
    output: screen
```

### With Dependencies

```yaml
entities:
  - type: execute_process
    name: database
    cmd: [echo, "Starting database..."]
    output: screen

  - type: execute_process
    name: api_server
    cmd: [echo, "Starting API server..."]
    output: screen
    depends_on:
      - database
```

### Variable Substitution

```yaml
entities:
  - type: declare_launch_argument
    name: message
    default_value: "Hello World"

  - type: execute_process
    cmd:
      - echo
      - $(var message)  # Expands to "Hello World"
```

## Safety Architecture (ISO 26262 ASIL B)

### OSAL Layer

The Operating System Abstraction Layer provides safety-critical interfaces:

- **ProcessExecutor**: Safe process execution with monitoring
- **ResourceMonitor**: Memory and CPU tracking
- **Watchdog**: Heartbeat monitoring for process health
- **RetryPolicy**: Configurable retry mechanisms

### DependencyResolver

Topological sorting for correct startup order:
- Kahn's algorithm (O(V+E) complexity)
- Circular dependency detection
- Parallel startup support

### Error Handling

AUTOSAR C++14 compliant error handling:

```cpp
Result<void> result = action.Execute(context);
if (result.HasError()) {
    std::cerr << "Error: " << result.GetError().GetMessage() << std::endl;
    return 1;
}
```

## Performance

- **Startup Time**: <10ms for typical launch files
- **Memory Overhead**: <1MB base
- **Process Spawn**: ~1ms (POSIX systems)

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

### Code Style

- 2-space indentation
- Google C++ style with modifications
- 100 character line limit
- Header guards: `LAUNCH_CPP__<PATH>__HPP_`
- Namespaces: `launch_cpp`

## License

Apache License 2.0 - See [LICENSE](LICENSE) for details.

## Acknowledgments

- ROS2 Launch System design
- AUTOSAR C++14 guidelines
- ISO 26262 functional safety standards

## Support

For issues and questions:
- GitHub Issues: [Report a bug](https://github.com/Nova-ROS/launch_cpp/issues)
- Documentation: See `docs/` directory
- Examples: See `examples/` directory

---

**Made with ❤️ for the ROS2 community**
