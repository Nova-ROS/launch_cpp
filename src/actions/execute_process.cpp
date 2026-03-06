// Copyright 2024 Example Author
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
  : Action(), options_(options), process_(nullptr)
{
}

ExecuteProcess::~ExecuteProcess() = default;

Result<void> ExecuteProcess::Execute(LaunchContext& context)
{
  // Resolve command
  std::vector<std::string> cmd;
  for (const auto& sub : options_.cmd)
  {
    if (sub)
    {
      cmd.push_back(sub->Perform(context));
    }
  }
  
  if (cmd.empty())
  {
    return Result<void>(Error(ErrorCode::kInvalidArgument, "Empty command"));
  }
  
  // Fork and exec
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

Error ExecuteProcess::Shutdown()
{
  if (process_ && process_->IsRunning())
  {
    kill(process_->GetPid(), SIGTERM);
  }
  return Error();
}

Error ExecuteProcess::Terminate()
{
  if (process_ && process_->IsRunning())
  {
    kill(process_->GetPid(), SIGTERM);
  }
  return Error();
}

Error ExecuteProcess::Kill()
{
  if (process_ && process_->IsRunning())
  {
    kill(process_->GetPid(), SIGKILL);
  }
  return Error();
}

void ExecuteProcess::SendSignal(std::int32_t signal)
{
  if (process_)
  {
    kill(process_->GetPid(), signal);
  }
}

bool ExecuteProcess::IsRunning() const noexcept
{
  if (!process_)
  {
    return false;
  }
  return process_->IsRunning();
}

Result<std::int32_t> ExecuteProcess::GetReturnCode() const
{
  if (!process_)
  {
    return Result<std::int32_t>(Error(ErrorCode::kInternalError, "Process not started"));
  }
  return Result<std::int32_t>(process_->GetReturnCode());
}

Result<std::int32_t> ExecuteProcess::GetPid() const
{
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

}  // namespace cpp_launch
