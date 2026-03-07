cmake_minimum_required(VERSION 3.14)
project(cpp_launch VERSION 0.1.0 LANGUAGES CXX)

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
  include/cpp_launch/safety
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
  # Safety architecture sources
  src/safety/posix_process_executor.cpp
  src/safety/posix_resource_monitor.cpp
  src/safety/posix_watchdog.cpp
  src/safety/mock_osal.cpp
)

add_library(cpp_launch_core ${CPP_LAUNCH_CORE_SOURCES})

target_include_directories(cpp_launch_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Executable
add_executable(cpp_launch_exec src/main.cpp)
set_target_properties(cpp_launch_exec PROPERTIES OUTPUT_NAME cpp_launch)
target_link_libraries(cpp_launch_exec cpp_launch_core)

# Install
install(TARGETS
  cpp_launch_core
  EXPORT cpp_launch
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(EXPORT cpp_launch
  FILE cpp_launchTargets.cmake
  DESTINATION share/cpp_launch/cmake
)

ament_export_targets(cpp_launch)
ament_export_include_directories(include)
ament_export_libraries(cpp_launch_core)

install(DIRECTORY include/
  DESTINATION include
)

# Debug executable for YAML parser
add_executable(test_yaml_debug test/test_yaml_debug.cpp)
target_link_libraries(test_yaml_debug cpp_launch_core)

# Testing
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  
  # Comprehensive test suites
  ament_add_gtest(test_yaml_parser test/test_yaml_parser.cpp)
  target_link_libraries(test_yaml_parser cpp_launch_core)
  
  ament_add_gtest(test_thread_pool_comprehensive test/test_thread_pool.cpp)
  target_link_libraries(test_thread_pool_comprehensive cpp_launch_core)
  
  ament_add_gtest(test_actions_comprehensive test/test_actions.cpp)
  target_link_libraries(test_actions_comprehensive cpp_launch_core)
  
  ament_add_gtest(test_substitutions_comprehensive test/test_substitutions.cpp)
  target_link_libraries(test_substitutions_comprehensive cpp_launch_core)
  
  ament_add_gtest(test_conditions_comprehensive test/test_conditions.cpp)
  target_link_libraries(test_conditions_comprehensive cpp_launch_core)
  
  ament_add_gtest(test_event_system_comprehensive test/test_event_system.cpp)
  target_link_libraries(test_event_system_comprehensive cpp_launch_core)
  
  ament_add_gtest(test_launch_service_comprehensive test/test_launch_service.cpp)
  target_link_libraries(test_launch_service_comprehensive cpp_launch_core)
  
  # Keep original simple test
  ament_add_gtest(test_integration test/simple_test.cpp)
  target_link_libraries(test_integration cpp_launch_core)

  # Extended tests for low coverage files
  ament_add_gtest(test_substitutions_extended test/test_substitutions_extended.cpp)
  target_link_libraries(test_substitutions_extended cpp_launch_core)

  ament_add_gtest(test_conditions_extended test/test_conditions_extended.cpp)
  target_link_libraries(test_conditions_extended cpp_launch_core)
endif()

ament_package()
