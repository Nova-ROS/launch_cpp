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


#include "cpp_launch/actions/execute_process.hpp"
#include "cpp_launch/launch_context.hpp"
#include "cpp_launch/substitution.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <iostream>

namespace cpp_launch
{

class Process
{
 public:
  explicit Process(pid_t pid) : pid_(pid), returnCode_(-1) {}
  
  pid_t GetPid() const { return pid_; }
  
  int Wait()
  {
    int status;
    waitpid(pid_, &status, 0);
    
    if (WIFEXITED(status))
    {
      returnCode_ = WEXITSTATUS(status);
      return returnCode_;
    }
    
    return -1;
  }
  
  bool IsRunning() const
  {
    int status;
    pid_t result = waitpid(pid_, &status, WNOHANG);
    return result == 0;
  }
  
  int GetReturnCode() const { return returnCode_; }
  
 private:
  pid_t pid_;
  int returnCode_;
};

ExecuteProcess::ExecuteProcess(const Options& options)
  : Action(), options_(options), process_(nullptr), processId_(0)
{
  // Initialize safety components if enabled
  if (options_.enableSafety)
  {
    // By default, use Posix implementations
    processExecutor_ = std::make_shared<cpp_launch::PosixProcessExecutor>();
    resourceMonitor_ = std::make_shared<cpp_launch::PosixResourceMonitor>();
    watchdog_ = std::make_shared<cpp_launch::PosixWatchdog>();
    
    // Start watchdog if timeout is configured
    if (options_.watchdogTimeoutMs > 0 && watchdog_)
    {
      watchdog_->Start();
    }
  }
}

ExecuteProcess::~ExecuteProcess()
{
  // Unregister from watchdog if enabled
  if (watchdog_ && processId_ != 0)
  {
    watchdog_->UnregisterNode(static_cast<uint32_t>(processId_));
  }
  
  // Stop watchdog
  if (watchdog_)
  {
    watchdog_->Stop();
  }
}

std::vector<std::string> ExecuteProcess::ResolveCommand(LaunchContext& context) const
{
  std::vector<std::string> cmd;
  for (const auto& sub : options_.cmd)
  {
    if (sub)
    {
      cmd.push_back(sub->Perform(context));
    }
  }
  return cmd;
}

Result<void> ExecuteProcess::Execute(LaunchContext& context)
{
  // Resolve command
  std::vector<std::string> cmd = ResolveCommand(context);
  
  if (cmd.empty())
  {
    return Result<void>(Error(ErrorCode::kInvalidArgument, "Empty command"));
  }
  
  // Safety: Check resources if safety is enabled
  if (options_.enableSafety && resourceMonitor_)
  {
    // Estimate memory requirement (simplified: use configured max or 100MB default)
    std::uint64_t estimatedMemory = options_.maxMemoryBytes > 0 ? 
                                    options_.maxMemoryBytes : 100 * 1024 * 1024;
    
    auto result = resourceMonitor_->AreResourcesAvailable(estimatedMemory);
    if (!result.IsSuccess() || !result.GetValue())
    {
      return Result<void>(Error(ErrorCode::kProcessSpawnFailed, 
                                "Insufficient resources to start process"));
    }
  }
  
  // Use safety-enabled execution if available
  if (options_.enableSafety && processExecutor_)
  {
    // Build OSAL command line
    cpp_launch::CommandLine command;
    if (!cmd.empty())
    {
      command.program = cmd[0];
      for (size_t i = 1; i < cmd.size(); ++i)
      {
        command.arguments.push_back(cmd[i]);
      }
    }
    
    // Build options
    cpp_launch::ProcessOptions processOptions;
    processOptions.startup_timeout = std::chrono::milliseconds(5000);
    processOptions.shutdown_timeout = std::chrono::seconds(options_.sigtermTimeout);
    processOptions.capture_stdout = (options_.output != "log");
    processOptions.capture_stderr = (options_.output != "log");
    
    // Execute using safety executor
    auto result = processExecutor_->Execute(command, processOptions);
    if (!result.IsSuccess())
    {
      return Result<void>(Error(ErrorCode::kProcessSpawnFailed, 
                                result.GetErrorMessage()));
    }
    
    processId_ = result.GetValue();
    
    // Register with watchdog if enabled
    if (watchdog_ && options_.watchdogTimeoutMs > 0)
    {
      auto regResult = watchdog_->RegisterNode(
        static_cast<uint32_t>(processId_), 
        static_cast<uint32_t>(options_.watchdogTimeoutMs),
        nullptr);
      
      if (!regResult.IsSuccess())
      {
        std::cerr << "Warning: Failed to register process with watchdog: "
                  << regResult.GetErrorMessage() << std::endl;
      }
    }
    
    // Set resource limits if configured
    if (resourceMonitor_ && (options_.maxMemoryBytes > 0 || options_.maxCpuPercent > 0))
    {
      resourceMonitor_->SetResourceLimits(
        processId_,
        options_.maxMemoryBytes,
        options_.maxCpuPercent);
    }
    
    // For screen output, wait for process to complete
    if (options_.output == "screen")
    {
      auto waitResult = processExecutor_->Wait(
        processId_, 
        std::chrono::milliseconds(-1)); // Wait indefinitely
      
      if (waitResult.IsSuccess())
      {
        const auto& processResult = waitResult.GetValue();
        (void)processResult; // Process completed
      }
    }
    
    // Also create legacy Process object for backward compatibility
    process_ = std::make_unique<Process>(static_cast<pid_t>(processId_));
    
    return Result<void>();
  }
  else
  {
    // Legacy execution path (original implementation)
    pid_t pid = fork();
    
    if (pid < 0)
    {
      return Result<void>(Error(ErrorCode::kProcessSpawnFailed, "Fork failed"));
    }
    
    if (pid == 0)
    {
      // Child process
      std::vector<char*> args;
      for (auto& arg : cmd)
      {
        args.push_back(&arg[0]);
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
      int returnCode = process_->Wait();
      (void)returnCode;
    }
    
    return Result<void>();
  }
}

Error ExecuteProcess::Shutdown()
{
  if (options_.enableSafety && processExecutor_ && processId_ != 0)
  {
    auto result = processExecutor_->Terminate(
      processId_, 
      std::chrono::seconds(options_.sigtermTimeout));
    return result.IsSuccess() ? Error() : 
           Error(ErrorCode::kInternalError, result.GetErrorMessage());
  }
  
  // Legacy path
  if (process_ && process_->IsRunning())
  {
    kill(process_->GetPid(), SIGTERM);
  }
  return Error();
}

Error ExecuteProcess::Terminate()
{
  if (options_.enableSafety && processExecutor_ && processId_ != 0)
  {
    auto result = processExecutor_->Terminate(
      processId_,
      std::chrono::seconds(options_.sigtermTimeout));
    return result.IsSuccess() ? Error() : 
           Error(ErrorCode::kInternalError, result.GetErrorMessage());
  }
  
  // Legacy path
  if (process_ && process_->IsRunning())
  {
    kill(process_->GetPid(), SIGTERM);
  }
  return Error();
}

Error ExecuteProcess::Kill()
{
  if (options_.enableSafety && processExecutor_ && processId_ != 0)
  {
    auto result = processExecutor_->Kill(processId_);
    return result.IsSuccess() ? Error() : 
           Error(ErrorCode::kInternalError, result.GetErrorMessage());
  }
  
  // Legacy path
  if (process_ && process_->IsRunning())
  {
    kill(process_->GetPid(), SIGKILL);
  }
  return Error();
}

void ExecuteProcess::SendSignal(std::int32_t signal)
{
  if (options_.enableSafety && processExecutor_ && processId_ != 0)
  {
    processExecutor_->SendSignal(processId_, signal);
    return;
  }
  
  // Legacy path
  if (process_)
  {
    kill(process_->GetPid(), signal);
  }
}

bool ExecuteProcess::IsRunning() const noexcept
{
  if (options_.enableSafety && processExecutor_ && processId_ != 0)
  {
    auto result = processExecutor_->IsRunning(processId_);
    if (result.IsSuccess())
    {
      return result.GetValue();
    }
    return false;
  }
  
  // Legacy path
  if (!process_)
  {
    return false;
  }
  return process_->IsRunning();
}

Result<std::int32_t> ExecuteProcess::GetReturnCode() const
{
  if (options_.enableSafety && processId_ != 0)
  {
    // For safety-enabled processes, we'd need to track return codes separately
    // For now, return the legacy path result if available
    if (process_)
    {
      return Result<std::int32_t>(process_->GetReturnCode());
    }
    return Result<std::int32_t>(-1);
  }
  
  // Legacy path
  if (!process_)
  {
    return Result<std::int32_t>(Error(ErrorCode::kInternalError, "Process not started"));
  }
  return Result<std::int32_t>(process_->GetReturnCode());
}

Result<std::int32_t> ExecuteProcess::GetPid() const
{
  if (options_.enableSafety && processId_ != 0)
  {
    return Result<std::int32_t>(static_cast<std::int32_t>(processId_));
  }
  
  // Legacy path
  if (!process_)
  {
    return Result<std::int32_t>(Error(ErrorCode::kInternalError, "Process not started"));
  }
  return Result<std::int32_t>(process_->GetPid());
}

std::string ExecuteProcess::GetName() const
{
  return resolvedName_;
}

void ExecuteProcess::SetProcessExecutor(std::shared_ptr<cpp_launch::ProcessExecutor> executor)
{
  processExecutor_ = executor;
}

void ExecuteProcess::SetResourceMonitor(std::shared_ptr<cpp_launch::ResourceMonitor> monitor)
{
  resourceMonitor_ = monitor;
}

void ExecuteProcess::SetWatchdog(std::shared_ptr<cpp_launch::Watchdog> watchdog)
{
  watchdog_ = watchdog;
}

bool ExecuteProcess::CheckResourcesAvailable(std::uint64_t estimatedMemory) const
{
  if (!resourceMonitor_)
  {
    return true;  // No monitor, assume resources available
  }
  
  auto result = resourceMonitor_->AreResourcesAvailable(estimatedMemory);
  return result.IsSuccess() && result.GetValue();
}

}  // namespace cpp_launch
