# cpp_launch - Zero External Dependencies Design

## Overview

This document describes the design principle of **zero external dependencies** for the cpp_launch project. The only dependency is the C++ standard library.

## Rationale

### Why Zero Dependencies?

1. **Deployment Simplicity**: No need to install or manage external libraries
2. **Reduced Attack Surface**: Fewer external dependencies means fewer potential security vulnerabilities
3. **Better Control**: Full control over all code, easier to debug and optimize
4. **AUTOSAR Compliance**: Easier to achieve safety certifications without external dependencies
5. **Portability**: Works on any platform with a C++14 compiler

## Self-Implemented Components

### 1. YAML Parser

**Instead of**: yaml-cpp

**Implementation**: Custom lightweight YAML parser (`src/yaml_parser.cpp`)

**Features**:
- Object, array, and scalar parsing
- Nested structures
- Comments and empty line handling
- File and string parsing

**Code Size**: ~450 lines

**Performance**: Sufficient for launch file parsing

### 2. Thread Pool

**Instead of**: Threading libraries

**Implementation**: Custom thread pool (`src/thread_pool.cpp`)

**Features**:
- Fixed-size thread pool
- Task queue with mutex and condition variable
- Graceful shutdown

**Code Size**: ~100 lines

### 3. Error Handling

**Instead of**: Exception-based error handling

**Implementation**: Result<T> pattern (`include/cpp_launch/error_code.hpp`)

**Features**:
- AUTOSAR C++14 compliant (no exceptions)
- Type-safe error propagation
- Clear error codes and messages

**Code Size**: ~150 lines

### 4. Process Management

**Instead of**: Process management libraries

**Implementation**: POSIX fork/exec wrapper (`src/actions/execute_process.cpp`)

**Features**:
- Process spawning
- Signal handling
- Basic I/O redirection

**Code Size**: ~150 lines

## Standard Library Only

All functionality is implemented using only:

```cpp
// Core headers
#include <memory>      // smart pointers
#include <vector>      // containers
#include <map>         // associative containers
#include <string>      // string handling
#include <thread>      // threading
#include <mutex>       // synchronization
#include <condition_variable>  // thread coordination
#include <atomic>      // atomic operations
#include <functional>  // function objects
#include <algorithm>   // algorithms
#include <sstream>     // string streams
#include <fstream>     // file I/O
#include <iostream>    // console I/O
#include <chrono>      // time utilities
#include <cstdint>     // fixed-width integers
```

## Comparison with Dependencies

| Component | External Option | Self-Implementation | Lines of Code |
|-----------|----------------|---------------------|---------------|
| YAML Parser | yaml-cpp (~12KB) | Custom (~450 LOC) | 450 |
| Thread Pool | Boost.Asio | Custom (~100 LOC) | 100 |
| Error Handling | Exceptions | Result<T> (~150 LOC) | 150 |
| Process Mgmt | libprocess | POSIX (~150 LOC) | 150 |
| **Total** | **~12KB+** | **~850 LOC** | **850** |

**Advantages**:
- Total control over implementation
- No external build dependencies
- Easier static analysis
- Better integration with AUTOSAR guidelines

**Trade-offs**:
- More code to maintain
- Less feature-complete than mature libraries
- Need to handle edge cases ourselves

## Build Requirements

```bash
# Minimum requirements - no external dependencies!
- CMake >= 3.14
- C++14 compiler (GCC 7+ or Clang 5+)
- POSIX-compliant system (Linux, macOS)
```

```bash
# Build command
colcon build --packages-select cpp_launch

# That's it! No need to install any libraries.
```

## Future Considerations

If external dependencies are needed in the future:

1. **Evaluate Carefully**: Only add if essential functionality cannot be implemented
2. **Header-Only Libraries**: Prefer header-only to avoid build complexity
3. **Standard Library Extensions**: Prefer C++17/20 features when available
4. **Vendor Branching**: Consider vendoring critical dependencies

## Verification

To verify zero dependencies:

```bash
# Check dynamic dependencies
ldd ./build/cpp_launch/cpp_launch

# Should only show system libraries:
# - libc.so
# - libpthread.so
# - libstdc++.so
# No yaml-cpp or other external libraries!
```

## Conclusion

The zero-dependency approach ensures:
- ✅ Simple deployment
- ✅ Full code control
- ✅ AUTOSAR compliance
- ✅ Easy auditing
- ✅ Reduced maintenance complexity

**Project Philosophy**: "If it's simple enough, implement it ourselves."

---

**Document Version**: 1.0  
**Last Updated**: 2024  
**Status**: Active
