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

#ifndef LAUNCH_CPP_COMMAND_BUILDER_HPP_
#define LAUNCH_CPP_COMMAND_BUILDER_HPP_

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "osal.hpp"
#include "launch_cpp/substitution.hpp"

namespace launch_cpp {

/**
 * @brief Options for node action (refactored from original)
 */
struct NodeActionOptions {
    // Node identification
    std::string package;
    std::string executable;
    std::string name;
    std::string namespace_;

    // Node configuration
    std::map<std::string, std::shared_ptr<Substitution>> parameters;
    std::map<std::string, std::string> remappings;
    std::vector<std::string> arguments;

    // Execution options
    std::string cwd;

    // Validation
    bool is_valid() const;
    std::vector<std::string> get_validation_errors() const;
};

/**
 * @brief Status codes for command building operations
 */
enum class BuildStatus {
    kSuccess,
    kInvalidPackage,
    kInvalidExecutable,
    kInvalidName,
    kInvalidParameter,
    kPathNotFound,
    kSystemError
};

/**
 * @brief Result type for command building operations (non-void)
 */
template<typename T>
class BuildResult {
public:
    BuildResult() : status_(BuildStatus::kSuccess) {}
    explicit BuildResult(T value) : status_(BuildStatus::kSuccess), value_(std::move(value)) {}
    explicit BuildResult(BuildStatus status) : status_(status) {}
    BuildResult(BuildStatus status, const std::string& message)
        : status_(status), error_message_(message) {}

    bool is_success() const { return status_ == BuildStatus::kSuccess; }
    bool has_error() const { return status_ != BuildStatus::kSuccess; }

    BuildStatus get_status() const { return status_; }
    const std::string& get_error_message() const { return error_message_; }

    T& get_value() { return value_; }
    const T& get_value() const { return value_; }

private:
    BuildStatus status_;
    T value_;
    std::string error_message_;
};

/**
 * @brief Specialization of BuildResult for void type
 */
template<>
class BuildResult<void> {
public:
    BuildResult() : status_(BuildStatus::kSuccess) {}
    explicit BuildResult(BuildStatus status) : status_(status) {}
    BuildResult(BuildStatus status, const std::string& message)
        : status_(status), error_message_(message) {}

    bool is_success() const { return status_ == BuildStatus::kSuccess; }
    bool has_error() const { return status_ != BuildStatus::kSuccess; }

    BuildStatus get_status() const { return status_; }
    const std::string& get_error_message() const { return error_message_; }

private:
    BuildStatus status_;
    std::string error_message_;
};

/**
 * @brief Command line builder - pure business logic, 100% testable
 *
 * This class contains NO system calls, NO fork/exec, NO file operations.
 * All I/O is passed in through parameters or returned as results.
 *
 * ISO 26262 Compliance:
 * - ASIL B: Business logic
 * - Testable: Yes (100% coverage target)
 * - Exceptions: No (uses Result<T> pattern)
 */
class CommandBuilder {
public:
    CommandBuilder() = default;
    ~CommandBuilder() = default;

    // Non-copyable (no need to copy builders)
    CommandBuilder(const CommandBuilder&) = delete;
    CommandBuilder& operator=(const CommandBuilder&) = delete;

    // Movable
    CommandBuilder(CommandBuilder&&) = default;
    CommandBuilder& operator=(CommandBuilder&&) = default;

    /**
     * @brief Build complete command line for a node
     * @param options Node configuration options
     * @param context Launch context for substitution evaluation
     * @return BuildResult containing CommandLine or error
     *
     * @pre options must be valid (check with ValidateOptions)
     * @post On success, returns valid CommandLine
     * @post On failure, returns error status and message
     *
     * @thread_safety Thread-safe (const method)
     */
    BuildResult<launch_cpp::CommandLine> Build(
        const NodeActionOptions& options,
        const LaunchContext& context) const;

    /**
     * @brief Validate options without building
     * @param options Node configuration options
     * @return BuildResult with success or validation errors
     *
     * @requirement TSR-001: Configuration validation
     */
    BuildResult<void> ValidateOptions(const NodeActionOptions& options) const;

    /**
     * @brief Build just the program path (ros2 run)
     * @return Path to ros2 executable or "ros2"
     *
     * @note Checks ROS2_PATH environment variable
     */
    std::string BuildProgramPath() const;

    /**
     * @brief Build node identification arguments
     * @param options Node options
     * @return Arguments for package and executable
     */
    std::vector<std::string> BuildNodeIdentification(
        const NodeActionOptions& options) const;

    /**
     * @brief Build name and namespace arguments
     * @param options Node options
     * @return Arguments for __node and __ns remappings
     */
    std::vector<std::string> BuildNameAndNamespace(
        const NodeActionOptions& options) const;

    /**
     * @brief Build parameter arguments
     * @param parameters Parameter map
     * @param context Launch context for substitution
     * @return Arguments for -p parameters
     *
     * @requirement TSR-001.5: Parameter type validation
     */
    BuildResult<std::vector<std::string>> BuildParameters(
        const std::map<std::string, std::shared_ptr<Substitution>>& parameters,
        const LaunchContext& context) const;

    /**
     * @brief Build remapping arguments
     * @param remappings Remapping map
     * @return Arguments for -r remappings
     */
    std::vector<std::string> BuildRemappings(
        const std::map<std::string, std::string>& remappings) const;

    /**
     * @brief Build extra arguments
     * @param arguments Extra argument list
     * @return Arguments vector
     */
    std::vector<std::string> BuildExtraArguments(
        const std::vector<std::string>& arguments) const;

    /**
     * @brief Validate package name
     * @param package Package name
     * @return true if valid
     *
     * Rules:
     * - Non-empty
     * - No spaces
     * - No special characters except _ and -
     */
    bool is_valid_package_name(const std::string& package) const;

    /**
     * @brief Validate executable name
     * @param executable Executable name
     * @return true if valid
     */
    bool is_valid_executable_name(const std::string& executable) const;

    /**
     * @brief Validate node name
     * @param name Node name
     * @return true if valid (or empty)
     */
    bool is_valid_node_name(const std::string& name) const;

    /**
     * @brief Validate parameter name
     * @param name Parameter name
     * @return true if valid
     */
    bool is_valid_parameter_name(const std::string& name) const;

    /**
     * @brief Escape special characters in argument
     * @param arg Argument string
     * @return Escaped string
     *
     * @note Public for use by ExecuteProcess
     */
    std::string escape_argument(const std::string& arg) const;

    /**
     * @brief Check if string contains spaces or special chars
     * @param str String to check
     * @return true if quoting needed
     *
     * @note Public for use by ExecuteProcess
     */
    bool needs_quoting(const std::string& str) const;
};

// ============================================================================
// Inline Implementation - Simple Methods
// ============================================================================

inline bool CommandBuilder::is_valid_package_name(const std::string& package) const {
    if (package.empty()) {
        return false;
    }

    // Package name rules:
    // - Must start with letter
    // - Can contain letters, numbers, underscores, hyphens
    // - No spaces
    if (!std::isalpha(package[0])) {
        return false;
    }

    for (char c : package) {
        if (!std::isalnum(c) && c != '_' && c != '-') {
            return false;
        }
    }

    return true;
}

inline bool CommandBuilder::is_valid_executable_name(const std::string& executable) const {
    if (executable.empty()) {
        return false;
    }

    // Similar to package name rules
    if (!std::isalpha(executable[0]) && executable[0] != '_') {
        return false;
    }

    for (char c : executable) {
        if (!std::isalnum(c) && c != '_' && c != '-') {
            return false;
        }
    }

    return true;
}

inline bool CommandBuilder::is_valid_node_name(const std::string& name) const {
    if (name.empty()) {
        return true;  // Empty is valid (will use default)
    }

    // Node name rules:
    // - Must start with letter or underscore
    // - Can contain letters, numbers, underscores
    // - No spaces, no hyphens in node names
    if (!std::isalpha(name[0]) && name[0] != '_') {
        return false;
    }

    for (char c : name) {
        if (!std::isalnum(c) && c != '_') {
            return false;
        }
    }

    return true;
}

inline bool CommandBuilder::is_valid_parameter_name(const std::string& name) const {
    if (name.empty()) {
        return false;
    }

    // Parameter name rules:
    // - Must start with letter or underscore
    // - Can contain letters, numbers, underscores, dots (for nested params)
    if (!std::isalpha(name[0]) && name[0] != '_') {
        return false;
    }

    for (char c : name) {
        if (!std::isalnum(c) && c != '_' && c != '.') {
            return false;
        }
    }

    return true;
}

inline bool CommandBuilder::needs_quoting(const std::string& str) const {
    for (char c : str) {
        if (std::isspace(c) || c == '"' || c == '\\' || c == '$' || c == '`') {
            return true;
        }
    }
    return false;
}

inline std::string CommandBuilder::escape_argument(const std::string& arg) const {
    if (!needs_quoting(arg)) {
        return arg;
    }

    // Simple quoting: wrap in double quotes, escape existing quotes
    std::string result = "\"";
    for (char c : arg) {
        if (c == '"' || c == '\\') {
            result += '\\';
        }
        result += c;
    }
    result += '"';
    return result;
}

// ============================================================================
// Non-Inline Implementations
// ============================================================================

inline BuildResult<void> CommandBuilder::ValidateOptions(
    const NodeActionOptions& options) const {

    if (!is_valid_package_name(options.package)) {
        return BuildResult<void>(
            BuildStatus::kInvalidPackage,
            "Invalid package name: " + options.package);
    }

    if (!is_valid_executable_name(options.executable)) {
        return BuildResult<void>(
            BuildStatus::kInvalidExecutable,
            "Invalid executable name: " + options.executable);
    }

    if (!is_valid_node_name(options.name)) {
        return BuildResult<void>(
            BuildStatus::kInvalidName,
            "Invalid node name: " + options.name);
    }

    return BuildResult<void>();
}

inline std::vector<std::string> CommandBuilder::BuildNodeIdentification(
    const NodeActionOptions& options) const {

    std::vector<std::string> args;
    args.push_back("run");
    args.push_back(options.package);
    args.push_back(options.executable);
    return args;
}

inline std::vector<std::string> CommandBuilder::BuildNameAndNamespace(
    const NodeActionOptions& options) const {

    std::vector<std::string> args;
    args.push_back("--ros-args");

    if (!options.name.empty()) {
        args.push_back("-r");
        args.push_back("__node:=" + options.name);
    }

    if (!options.namespace_.empty()) {
        args.push_back("-r");
        args.push_back("__ns:=" + options.namespace_);
    }

    return args;
}

// ============================================================================
// Unit Test Support Functions
// ============================================================================

#ifdef BUILD_TESTING
/**
 * @brief Test fixture for CommandBuilder tests
 *
 * Provides helper methods for testing CommandBuilder
 */
class CommandBuilderTestFixture {
public:
    CommandBuilder CreateBuilder() const {
        return CommandBuilder();
    }

    NodeActionOptions CreateValidOptions() const {
        NodeActionOptions options;
        options.package = "demo_nodes_cpp";
        options.executable = "talker";
        options.name = "test_talker";
        options.namespace_ = "/test_ns";
        return options;
    }

    NodeActionOptions CreateInvalidOptions() const {
        NodeActionOptions options;
        options.package = "";  // Invalid: empty
        options.executable = "talker";
        return options;
    }
};
#endif  // BUILD_TESTING

} // namespace launch_cpp

#endif // LAUNCH_CPP_COMMAND_BUILDER_HPP_
