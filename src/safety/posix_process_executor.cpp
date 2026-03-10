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


/**
 * @file posix_process_executor.cpp
 * @brief POSIX implementation of ProcessExecutor
 * 
 * ASIL: B (Simple adapter - code reviewed)
 * This is a thin wrapper around POSIX system calls.
 * Business logic is in NodeActionRefactored, not here.
 */

#include "osal.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <fcntl.h>

namespace launch_cpp {


// ============================================================================
// PosixProcessExecutor Implementation
// ============================================================================

class PosixProcessExecutor::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    OsalResult<ProcessId> ExecuteInternal(
        const CommandLine& command,
        const ProcessOptions& options);

    OsalResult<ProcessResult> WaitInternal(
        ProcessId pid,
        std::chrono::milliseconds timeout);

    OsalResult<bool> IsRunningInternal(ProcessId pid);
    OsalResult<void> TerminateInternal(ProcessId pid, std::chrono::milliseconds timeout);
    OsalResult<void> KillInternal(ProcessId pid);
    OsalResult<void> SendSignalInternal(ProcessId pid, int32_t signal);
    OsalResult<ProcessState> GetStateInternal(ProcessId pid);

private:
    // Convert CommandLine to argv array
    std::vector<char*> BuildArgv(const std::vector<std::string>& args);
    
    // Get program path (check ROS2_PATH env var)
    std::string resolve_program_path(const std::string& program);
    
    // Setup child process environment
    void setup_child_environment(const std::vector<std::pair<std::string, std::string>>& env);
};

// ============================================================================
// Public Interface
// ============================================================================

PosixProcessExecutor::PosixProcessExecutor() 
    : impl_(std::make_unique<Impl>()) {}

PosixProcessExecutor::~PosixProcessExecutor() = default;

OsalResult<ProcessId> PosixProcessExecutor::execute(
    const CommandLine& command,
    const ProcessOptions& options) {
    return impl_->ExecuteInternal(command, options);
}

OsalResult<ProcessResult> PosixProcessExecutor::wait(
    ProcessId pid,
    std::chrono::milliseconds timeout) {
    return impl_->WaitInternal(pid, timeout);
}

OsalResult<bool> PosixProcessExecutor::is_running(ProcessId pid) {
    return impl_->IsRunningInternal(pid);
}

OsalResult<void> PosixProcessExecutor::terminate(
    ProcessId pid,
    std::chrono::milliseconds timeout) {
    return impl_->TerminateInternal(pid, timeout);
}

OsalResult<void> PosixProcessExecutor::kill(ProcessId pid) {
    return impl_->KillInternal(pid);
}

OsalResult<void> PosixProcessExecutor::send_signal(ProcessId pid, int32_t signal) {
    return impl_->SendSignalInternal(pid, signal);
}

OsalResult<ProcessState> PosixProcessExecutor::get_state(ProcessId pid) {
    return impl_->GetStateInternal(pid);
}

// ============================================================================
// Implementation Details
// ============================================================================

OsalResult<ProcessId> PosixProcessExecutor::Impl::ExecuteInternal(
    const CommandLine& command,
    const ProcessOptions& options) {

    // Validate command
    if (command.program.empty()) {
        return OsalResult<ProcessId>(
            OsalStatus::kInvalidArgument,
            "Program path is empty");
    }

    // Resolve program path
    std::string program_path = resolve_program_path(command.program);

    // Build argument list
    std::vector<std::string> all_args = {program_path};
    all_args.insert(all_args.end(), command.arguments.begin(), command.arguments.end());
    std::vector<char*> argv = BuildArgv(all_args);

    // Fork child process
    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        return OsalResult<ProcessId>(
            OsalStatus::kError,
            std::string("Fork failed: ") + std::strerror(errno));
    }

    if (pid == 0) {
        // Child process
        
        // Change working directory if specified
        if (!command.working_directory.empty()) {
            if (chdir(command.working_directory.c_str()) != 0) {
                std::cerr << "Failed to change directory: " << std::strerror(errno) << std::endl;
                _exit(1);
            }
        }

        // Setup environment variables
        setup_child_environment(command.environment);

        // Redirect output if needed
        if (!options.capture_stdout) {
            int dev_null = open("/dev/null", O_WRONLY);
            if (dev_null >= 0) {
                dup2(dev_null, STDOUT_FILENO);
                close(dev_null);
            }
        }

        if (!options.capture_stderr) {
            int dev_null = open("/dev/null", O_WRONLY);
            if (dev_null >= 0) {
                dup2(dev_null, STDERR_FILENO);
                close(dev_null);
            }
        }

        // Execute program
        execvp(argv[0], argv.data());

        // If we get here, exec failed
        std::cerr << "Failed to execute " << argv[0] << ": " << std::strerror(errno) << std::endl;
        _exit(127);
    }

    // Parent process - child started successfully
    return OsalResult<ProcessId>(static_cast<ProcessId>(pid));
}

OsalResult<ProcessResult> PosixProcessExecutor::Impl::WaitInternal(
    ProcessId pid,
    std::chrono::milliseconds timeout) {

    auto start_time = std::chrono::steady_clock::now();
    int status = 0;
    pid_t result = 0;

    // Wait with timeout
    if (timeout.count() > 0) {
        // Non-blocking wait with polling
        while (true) {
            result = waitpid(static_cast<pid_t>(pid), &status, WNOHANG);
            
            if (result == pid) {
                // Process exited
                break;
            } else if (result == 0) {
                // Still running, check timeout
                auto elapsed = std::chrono::steady_clock::now() - start_time;
                if (elapsed >= timeout) {
                    return OsalResult<ProcessResult>(
                        OsalStatus::kTimeout,
                        "Wait timeout");
                }
                // Short sleep to avoid busy waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } else {
                // Error
                return OsalResult<ProcessResult>(
                    OsalStatus::kError,
                    std::string("Wait failed: ") + std::strerror(errno));
            }
        }
    } else {
        // Infinite wait
        result = waitpid(static_cast<pid_t>(pid), &status, 0);
        if (result < 0) {
            return OsalResult<ProcessResult>(
                OsalStatus::kError,
                std::string("Wait failed: ") + std::strerror(errno));
        }
    }

    // Build result
    ProcessResult proc_result;
    proc_result.pid = pid;
    proc_result.end_time = std::chrono::steady_clock::now();

    if (WIFEXITED(status)) {
        proc_result.exit_code = WEXITSTATUS(status);
        proc_result.final_state = (proc_result.exit_code == 0) 
            ? ProcessState::kStopped 
            : ProcessState::kCrashed;
    } else if (WIFSIGNALED(status)) {
        proc_result.exit_code = -WTERMSIG(status);
        proc_result.final_state = ProcessState::kCrashed;
    } else {
        proc_result.exit_code = -1;
        proc_result.final_state = ProcessState::kUnknown;
    }

    return OsalResult<ProcessResult>(proc_result);
}

OsalResult<bool> PosixProcessExecutor::Impl::IsRunningInternal(ProcessId pid) {
    // Use kill with signal 0 to check if process exists
    int result = ::kill(static_cast<pid_t>(pid), 0);
    
    if (result == 0) {
        return OsalResult<bool>(true);
    } else {
        if (errno == ESRCH) {
            // Process doesn't exist
            return OsalResult<bool>(false);
        } else if (errno == EPERM) {
            // Process exists but no permission
            return OsalResult<bool>(true);
        } else {
            return OsalResult<bool>(
                OsalStatus::kError,
                std::string("Check failed: ") + std::strerror(errno));
        }
    }
}

OsalResult<void> PosixProcessExecutor::Impl::TerminateInternal(
    ProcessId pid,
    std::chrono::milliseconds timeout) {

    // Send SIGTERM
    int result = ::kill(static_cast<pid_t>(pid), SIGTERM);
    if (result < 0) {
        if (errno == ESRCH) {
            // Process already gone
            return OsalResult<void>();
        }
        return OsalResult<void>(
            OsalStatus::kError,
            std::string("SIGTERM failed: ") + std::strerror(errno));
    }

    // Wait for graceful termination
    auto start = std::chrono::steady_clock::now();
    int status = 0;

    while (true) {
        pid_t wait_result = waitpid(static_cast<pid_t>(pid), &status, WNOHANG);
        
        if (wait_result == pid) {
            // Process exited gracefully
            return OsalResult<void>();
        } else if (wait_result == 0) {
            // Still running, check timeout
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout) {
                // Timeout - force kill
                ::kill(static_cast<pid_t>(pid), SIGKILL);
                // Wait for it to die
                waitpid(static_cast<pid_t>(pid), &status, 0);
                return OsalResult<void>();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            // Error
            break;
        }
    }

    return OsalResult<void>();
}

OsalResult<void> PosixProcessExecutor::Impl::KillInternal(ProcessId pid) {
    int result = ::kill(static_cast<pid_t>(pid), SIGKILL);
    
    if (result < 0) {
        if (errno == ESRCH) {
            // Already gone
            return OsalResult<void>();
        }
        return OsalResult<void>(
            OsalStatus::kError,
            std::string("SIGKILL failed: ") + std::strerror(errno));
    }

    // Reap the zombie
    int status = 0;
    waitpid(static_cast<pid_t>(pid), &status, 0);
    
    return OsalResult<void>();
}

OsalResult<void> PosixProcessExecutor::Impl::SendSignalInternal(
    ProcessId pid, 
    int32_t signal) {

    int result = ::kill(static_cast<pid_t>(pid), static_cast<int>(signal));
    
    if (result < 0) {
        return OsalResult<void>(
            OsalStatus::kError,
            std::string("Send signal failed: ") + std::strerror(errno));
    }

    return OsalResult<void>();
}

OsalResult<ProcessState> PosixProcessExecutor::Impl::GetStateInternal(ProcessId pid) {
    auto running_result = IsRunningInternal(pid);
    if (running_result.has_error()) {
        return OsalResult<ProcessState>(
            running_result.get_status(),
            running_result.get_error_message());
    }

    if (running_result.get_value()) {
        return OsalResult<ProcessState>(ProcessState::kRunning);
    } else {
        return OsalResult<ProcessState>(ProcessState::kStopped);
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

std::vector<char*> PosixProcessExecutor::Impl::BuildArgv(
    const std::vector<std::string>& args) {
    
    std::vector<char*> argv;
    for (auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);
    return argv;
}

std::string PosixProcessExecutor::Impl::resolve_program_path(
    const std::string& program) {
    
    // If program is already an absolute path, use it
    if (program[0] == '/') {
        return program;
    }

    // Check for ros2 command
    if (program == "ros2") {
        const char* ros2_path = std::getenv("ROS2_PATH");
        if (ros2_path != nullptr) {
            return std::string(ros2_path) + "/bin/ros2";
        }
    }

    // Return as-is (will be searched in PATH)
    return program;
}

void PosixProcessExecutor::Impl::setup_child_environment(
    const std::vector<std::pair<std::string, std::string>>& env) {
    
    for (const auto& [key, value] : env) {
        setenv(key.c_str(), value.c_str(), 1);  // 1 = overwrite
    }
}

} // namespace launch_cpp
