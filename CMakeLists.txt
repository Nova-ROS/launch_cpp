cmake_minimum_required(VERSION 3.14)
project(launch_cpp VERSION 0.1.0 LANGUAGES CXX)

# C++14 Standard (AUTOSAR Compliance)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)

# Include directories
include_directories(
  include
  src
  include/launch_cpp/safety
)

# Core library
set(CPP_LAUNCH_CORE_SOURCES
  src/thread_pool.cpp
  src/launch_description.cpp
  src/action.cpp
  src/launch_service.cpp
  src/yaml_parser.cpp
  src/actions/execute_process.cpp
  src/actions/declare_launch_argument.cpp
  src/actions/include_launch_description.cpp
  src/actions/set_launch_configuration.cpp
  src/actions/timer_action.cpp
  src/actions/group_action.cpp
  src/substitutions/launch_configuration.cpp
  src/substitutions/environment_variable.cpp
  src/substitutions/command.cpp
  src/substitutions/find_executable.cpp
  src/substitutions/this_launch_file.cpp
  src/substitutions/this_launch_file_dir.cpp
  src/conditions/if_condition.cpp
  src/conditions/launch_configuration_equals.cpp
  src/conditions/unless_condition.cpp
  src/dependency_manager.cpp
  # Safety architecture sources
  src/safety/posix_process_executor.cpp
  src/safety/posix_resource_monitor.cpp
  src/safety/posix_watchdog.cpp
  src/safety/mock_osal.cpp
)

add_library(launch_cpp_core ${CPP_LAUNCH_CORE_SOURCES})

target_include_directories(launch_cpp_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Executable
add_executable(launch_cpp_exec src/main.cpp)
set_target_properties(launch_cpp_exec PROPERTIES OUTPUT_NAME launch_cpp)
target_link_libraries(launch_cpp_exec launch_cpp_core)

# Install
install(TARGETS
  launch_cpp_core
  launch_cpp_exec
  EXPORT launch_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(EXPORT launch_cpp
  FILE launch_cppTargets.cmake
  DESTINATION share/launch_cpp/cmake
)

ament_export_targets(launch_cpp)
ament_export_include_directories(include)
ament_export_libraries(launch_cpp_core)

install(DIRECTORY include/
  DESTINATION include
)

# Debug executable for YAML parser
add_executable(test_yaml_debug test/test_yaml_debug.cpp)
target_link_libraries(test_yaml_debug launch_cpp_core)

# Examples
add_executable(example_basic examples/example_basic.cpp)
target_link_libraries(example_basic launch_cpp_core)

install(TARGETS
  example_basic
  DESTINATION lib/launch_cpp/examples
)

install(DIRECTORY examples/
  DESTINATION share/launch_cpp/examples
  FILES_MATCHING PATTERN "*.yaml"
)

# Testing
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  add_subdirectory(test)
endif()

ament_package()
