# cpp_launch - Modern C++ Implementation of ROS2 Launch

## Overview

This is a modern C++ implementation of the ROS2 launch system, compliant with AUTOSAR C++14 standards. It provides a high-performance, type-safe alternative to the Python-based launch system.

## Features

- **AUTOSAR C++14 Compliant**: No exceptions, type-safe, explicit memory management
- **High Performance**: Compiled C++14, minimal runtime overhead
- **Modular Design**: Clean separation of concerns
- **ROS2 Package**: Standard ament_cmake build system
- **Zero External Dependencies**: Self-contained implementation using only standard library

## Quick Start

### Build

```bash
cd ~/work/ros2/jazzy/design/cpp_launch
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Run Tests

```bash
make test
```

### Run Example

```bash
./cpp_launch ../examples/example_launch.yaml
```

### Use as Library

```cpp
#include "cpp_launch/launch_service.hpp"
#include "cpp_launch/launch_description.hpp"
#include "cpp_launch/actions/execute_process.hpp"
#include "cpp_launch/substitutions/text_substitution.hpp"

using namespace cpp_launch;

int main()
{
  LaunchService service;
  
  auto desc = std::make_shared<LaunchDescription>();
  
  ExecuteProcess::Options options;
  options.cmd.push_back(std::make_shared<TextSubstitution>("echo"));
  options.cmd.push_back(std::make_shared<TextSubstitution>("Hello!"));
  
  desc->Add(std::make_shared<ExecuteProcess>(options));
  
  service.IncludeLaunchDescription(desc);
  return service.Run();
}
```

## Project Structure

```
cpp_launch/
├── include/cpp_launch/          # Public headers
│   ├── types.hpp               # Basic types
│   ├── error_code.hpp          # Error handling
│   ├── launch_service.hpp      # Main service
│   ├── launch_description.hpp  # Launch description
│   ├── action.hpp              # Action base
│   ├── event.hpp               # Event system
│   ├── substitution.hpp        # Substitution interface
│   ├── condition.hpp           # Condition interface
│   └── actions/                # Action implementations
│   └── substitutions/          # Substitution implementations
│   └── conditions/             # Condition implementations
├── src/                        # Implementation files
├── test/                       # Unit tests
├── examples/                   # Example files
└── docs/                       # Documentation
```

## AUTOSAR C++14 Compliance

| Rule | Description | Implementation |
|------|-------------|----------------|
| A15-0-1 | No exceptions | `Result<T>` for error handling |
| A18-5-2 | No std::exception | Custom `Error` class |
| A7-2-4 | enum class | All enumerations |
| A12-8-4 | Virtual destructor | All base classes |
| M0-1-9 | noexcept | Appropriate functions |
| A10-3-3 | Special functions | All classes |
| A12-1-1 | Member initialization | All constructors |

## Dependencies

### Required
- CMake >= 3.14
- C++14 compiler (GCC 7+ or Clang 5+)
- **No external libraries** - Only C++ standard library

### Optional (for testing)
- Google Test (via ament_cmake_gtest)

## License

Apache 2.0

## Contributing

See `CONTRIBUTING.md` for details.
