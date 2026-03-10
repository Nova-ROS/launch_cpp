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


#include "launch_cpp/actions/execute_process.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/substitution.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <cstdint>

namespace launch_cpp
{

class Process
{
 public:
  explicit Process(pid_t pid) : pid_(pid), return_code_(-1) {}

  pid_t get_pid() const { return pid_; }

  int wait()
  {
    int status;
    waitpid(pid_, &status, 0);

    if (WIFEXITED(status))
    {
      return_code_ = WEXITSTATUS(status);
      return return_code_;
    }

    return -1;
  }

  bool is_running() const
  {
    int status;
    pid_t result = waitpid(pid_, &status, WNOHANG);
    return result == 0;
  }

  int get_return_code() const { return return_code_; }

 private:
  pid_t pid_;
  int return_code_;
};

ExecuteProcess::ExecuteProcess(const Options& options)
  : Action(), options_(options), process_(nullptr), process_id_(0)
{
  // Initialize safety components if enabled
  if (options_.enable_safety)
  {
    // By default, use Posix implementations
    process_executor_ = std::make_shared<launch_cpp::PosixProcessExecutor>();
    resource_monitor_ = std::make_shared<launch_cpp::PosixResourceMonitor>();
    watchdog_ = std::make_shared<launch_cpp::PosixWatchdog>();

    // Start watchdog if timeout is configured
    if (options_.watchdog_timeout_ms > 0 && watchdog_)
    {
      watchdog_->start();
    }
  }
}

ExecuteProcess::~ExecuteProcess()
{
  // Unregister from watchdog if enabled
  if (watchdog_ && process_id_ != 0)
  {
    watchdog_->unregister_node(static_cast<uint32_t>(process_id_));
  }

  // Stop watchdog
  if (watchdog_)
  {
    watchdog_->stop();
  }
}

std::vector<std::string> ExecuteProcess::resolve_command(LaunchContext& context) const
{
  std::vector<std::string> cmd;
  for (const auto& sub : options_.cmd)
  {
    if (sub)
    {
      cmd.push_back(sub->perform(context));
    }
  }
  return cmd;
}

Result<std::vector<std::string>> ExecuteProcess::validate_and_escape_command(
    const std::vector<std::string>& cmd) const
{
  if (cmd.empty())
  {
    return Result<std::vector<std::string>>(
        Error(ErrorCode::K_INVALID_ARGUMENT, "Empty command"));
  }

  // Use CommandBuilder for validation and escaping
  CommandBuilder builder;
  std::vector<std::string> validatedCmd;

  // Validate and escape each argument
  for (const auto& arg : cmd)
  {
    if (arg.empty())
    {
      return Result<std::vector<std::string>>(
          Error(ErrorCode::K_INVALID_ARGUMENT, "Empty argument in command"));
    }

    // Escape argument if needed
    std::string escapedArg = builder.escape_argument(arg);
    validatedCmd.push_back(escapedArg);
  }

  return Result<std::vector<std::string>>(validatedCmd);
}

Result<void> ExecuteProcess::execute_single_attempt(LaunchContext& context,
                                                    const std::vector<std::string>& cmd)
{
  // Safety: Check resources if safety is enabled
  if (options_.enable_safety && resource_monitor_)
  {
    // Estimate memory requirement (simplified: use configured max or 100MB default)
    std::uint64_t estimatedMemory = options_.max_memory_bytes > 0 ?
                                    options_.max_memory_bytes : 100 * 1024 * 1024;

    auto result = resource_monitor_->are_resources_available(estimatedMemory);
    if (!result.is_success() || !result.get_value())
    {
      return Result<void>(Error(ErrorCode::K_PROCESS_SPAWN_FAILED,
                                "Insufficient resources to start process"));
    }
  }

  // Use safety-enabled execution if available
  if (options_.enable_safety && process_executor_)
  {
    // Build OSAL command line
    launch_cpp::CommandLine command;
    if (!cmd.empty())
    {
      command.program = cmd[0];
      for (size_t i = 1; i < cmd.size(); ++i)
      {
        command.arguments.push_back(cmd[i]);
      }
    }

    // Build options
    launch_cpp::ProcessOptions processOptions;
    processOptions.startup_timeout = std::chrono::milliseconds(5000);
    processOptions.shutdown_timeout = std::chrono::seconds(options_.sigterm_timeout);
    processOptions.capture_stdout = (options_.output != "log");
    processOptions.capture_stderr = (options_.output != "log");

    // Execute using safety executor
    auto result = process_executor_->execute(command, processOptions);
    if (!result.is_success())
    {
      return Result<void>(Error(ErrorCode::K_PROCESS_SPAWN_FAILED,
                                result.get_error_message()));
    }

    process_id_ = result.get_value();

    // Register with watchdog if enabled
    if (watchdog_ && options_.watchdog_timeout_ms > 0)
    {
      auto regResult = watchdog_->register_node(
        static_cast<uint32_t>(process_id_),
        static_cast<uint32_t>(options_.watchdog_timeout_ms),
        nullptr);

      if (!regResult.is_success())
      {
        std::cerr << "Warning: Failed to register process with watchdog: "
                  << regResult.get_error_message() << std::endl;
      }
    }

    // Set resource limits if configured
    if (resource_monitor_ && (options_.max_memory_bytes > 0 || options_.max_cpu_percent > 0))
    {
      resource_monitor_->set_resource_limits(
        process_id_,
        options_.max_memory_bytes,
        options_.max_cpu_percent);
    }

    // For screen output, wait for process to complete
    if (options_.output == "screen")
    {
      auto waitResult = process_executor_->wait(
        process_id_,
        std::chrono::milliseconds(-1));  // Wait indefinitely

      if (waitResult.is_success())
      {
        const auto& processResult = waitResult.get_value();
        (void)processResult;  // Process completed
      }
    }

    // Also create legacy Process object for backward compatibility
    process_ = std::make_unique<Process>(static_cast<pid_t>(process_id_));

    return Result<void>();
  } else {
    // Legacy execution path (original implementation)
    pid_t pid = fork();

    if (pid < 0)
    {
      return Result<void>(Error(ErrorCode::K_PROCESS_SPAWN_FAILED, "Fork failed"));
    }

    if (pid == 0)
    {
      // Child process
      std::vector<char*> args;
      for (auto& arg : cmd)
      {
        // Safe const_cast: execvp doesn't modify args, and we exit immediately after
        args.push_back(const_cast<char*>(arg.c_str()));
      }
      args.push_back(nullptr);

      execvp(args[0], args.data());

      // If we get here, exec failed
      _exit(1);
    }

    // Parent process
    process_ = std::make_unique<Process>(pid);

    // Wait for process if output is set to screen
    if (options_.output == "screen")
    {
      int return_code = process_->wait();
      (void)return_code;
    }

    return Result<void>();
  }
}

Result<void> ExecuteProcess::execute(LaunchContext& context)
{
  // Resolve command from substitutions
  std::vector<std::string> rawCmd = resolve_command(context);

  // Validate and escape command using CommandBuilder
  auto validationResult = validate_and_escape_command(rawCmd);
  if (!validationResult.has_value())
  {
    return Result<void>(validationResult.get_error());
  }

  std::vector<std::string> cmd = validationResult.get_value();

  // Execute with retry logic if configured
  if (options_.max_retries > 0)
  {
    return execute_with_retry(context, cmd);
  }

  // Single attempt (no retry)
  return execute_single_attempt(context, cmd);
}

Result<void> ExecuteProcess::execute_with_retry(LaunchContext& context,
                                               const std::vector<std::string>& cmd)
{
  uint32_t attempt = 0;
  Result<void> lastResult;

  while (attempt <= options_.max_retries)
  {
    // Execute single attempt
    lastResult = execute_single_attempt(context, cmd);

    // Check if successful
    if (lastResult.has_value())
    {
      return lastResult;
    }

    // Check if we should retry
    attempt++;
    if (attempt > options_.max_retries)
    {
      break;
    }

    // Check if error is retryable
    if (!is_retryable_error(lastResult.get_error().get_code()))
    {
      return lastResult;
    }

    // Clean up before retry
    cleanup_before_retry();

    // Calculate delay for this attempt
    auto delay = calculate_retry_delay(attempt);

    std::cerr << "Process execution failed (attempt " << attempt
              << "/" << options_.max_retries + 1 << "), retrying in "
              << delay.count() << "ms..." << std::endl;

    // Sleep before retry
    std::this_thread::sleep_for(delay);
  }

  // All retries exhausted
  return Result<void>(Error(ErrorCode::K_MAX_RETRIES_EXCEEDED,
                            "Max retry attempts exceeded"));
}

bool ExecuteProcess::is_retryable_error(ErrorCode code) const
{
  // Retryable errors:
  // - Resource exhaustion (might be temporary)
  // - Fork failures (might recover)
  // - Timeout (might succeed next time)
  switch (code)
  {
    case ErrorCode::K_PROCESS_SPAWN_FAILED:
    case ErrorCode::K_TIMEOUT:
    case ErrorCode::K_RESOURCE_EXHAUSTED:
      return true;
    default:
      return false;
  }
}

std::chrono::milliseconds ExecuteProcess::calculate_retry_delay(uint32_t attemptNumber) const
{
  if (attemptNumber == 0 || options_.retry_backoff_multiplier <= 0.0)
  {
    return options_.retry_delay;
  }

  // Calculate exponential backoff: delay * multiplier^(attempt-1)
  double multiplier = 1.0;
  for (uint32_t i = 1; i < attemptNumber; ++i)
  {
    multiplier *= options_.retry_backoff_multiplier;
  }

  auto delayMs = static_cast<uint32_t>(options_.retry_delay.count() * multiplier);
  return std::chrono::milliseconds(delayMs);
}

void ExecuteProcess::cleanup_before_retry()
{
  // Clean up any resources from failed attempt
  if (process_)
  {
    if (process_->is_running())
    {
      ::kill(process_->get_pid(), SIGTERM);
      // Give it a moment to terminate
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    process_.reset();
  }

  // Unregister from watchdog if registered
  if (watchdog_ && process_id_ != 0)
  {
    watchdog_->unregister_node(static_cast<uint32_t>(process_id_));
  }

  // Reset process ID
  process_id_ = 0;
}

Error ExecuteProcess::shutdown()
{
  if (options_.enable_safety && process_executor_ && process_id_ != 0)
  {
    auto result = process_executor_->terminate(
      process_id_,
      std::chrono::seconds(options_.sigterm_timeout));
    return result.is_success() ? Error() :
           Error(ErrorCode::K_INTERNAL_ERROR, result.get_error_message());
  }

  // Legacy path
  if (process_ && process_->is_running())
  {
    ::kill(process_->get_pid(), SIGTERM);
  }
  return Error();
}

Error ExecuteProcess::terminate()
{
  if (options_.enable_safety && process_executor_ && process_id_ != 0)
  {
    auto result = process_executor_->terminate(
      process_id_,
      std::chrono::seconds(options_.sigterm_timeout));
    return result.is_success() ? Error() :
           Error(ErrorCode::K_INTERNAL_ERROR, result.get_error_message());
  }

  // Legacy path
  if (process_ && process_->is_running())
  {
    ::kill(process_->get_pid(), SIGTERM);
  }
  return Error();
}

Error ExecuteProcess::kill()
{
  if (options_.enable_safety && process_executor_ && process_id_ != 0)
  {
    auto result = process_executor_->kill(process_id_);
    return result.is_success() ? Error() :
           Error(ErrorCode::K_INTERNAL_ERROR, result.get_error_message());
  }

  // Legacy path
  if (process_ && process_->is_running())
  {
    ::kill(process_->get_pid(), SIGKILL);
  }
  return Error();
}

void ExecuteProcess::send_signal(std::int32_t signal)
{
  if (options_.enable_safety && process_executor_ && process_id_ != 0)
  {
    process_executor_->send_signal(process_id_, signal);
    return;
  }

  // Legacy path
  if (process_)
  {
    ::kill(process_->get_pid(), signal);
  }
}

bool ExecuteProcess::is_running() const noexcept
{
  if (options_.enable_safety && process_executor_ && process_id_ != 0)
  {
    auto result = process_executor_->is_running(process_id_);
    if (result.is_success())
    {
      return result.get_value();
    }
    return false;
  }

  // Legacy path
  if (!process_)
  {
    return false;
  }
  return process_->is_running();
}

Result<std::int32_t> ExecuteProcess::get_return_code() const
{
  if (options_.enable_safety && process_id_ != 0)
  {
    // For safety-enabled processes, we'd need to track return codes separately
    // For now, return the legacy path result if available
    if (process_)
    {
      return Result<std::int32_t>(process_->get_return_code());
    }
    return Result<std::int32_t>(-1);
  }

  // Legacy path
  if (!process_)
  {
    return Result<std::int32_t>(Error(ErrorCode::K_INTERNAL_ERROR, "Process not started"));
  }
  return Result<std::int32_t>(process_->get_return_code());
}

Result<std::int32_t> ExecuteProcess::get_pid() const
{
  if (options_.enable_safety && process_id_ != 0)
  {
    return Result<std::int32_t>(static_cast<std::int32_t>(process_id_));
  }

  // Legacy path
  if (!process_)
  {
    return Result<std::int32_t>(Error(ErrorCode::K_INTERNAL_ERROR, "Process not started"));
  }
  return Result<std::int32_t>(process_->get_pid());
}

std::string ExecuteProcess::get_name() const
{
  return resolved_name_;
}

void ExecuteProcess::set_process_executor(std::shared_ptr<launch_cpp::ProcessExecutor> executor)
{
  process_executor_ = executor;
}

void ExecuteProcess::set_resource_monitor(std::shared_ptr<launch_cpp::ResourceMonitor> monitor)
{
  resource_monitor_ = monitor;
}

void ExecuteProcess::set_watchdog(std::shared_ptr<launch_cpp::Watchdog> wd)
{
  watchdog_ = wd;
}

bool ExecuteProcess::check_resources_available(std::uint64_t estimatedMemory) const
{
  if (!resource_monitor_)
  {
    return true;  // No monitor, assume resources available
  }

  auto result = resource_monitor_->are_resources_available(estimatedMemory);
  return result.is_success() && result.get_value();
}

}  // namespace launch_cpp
