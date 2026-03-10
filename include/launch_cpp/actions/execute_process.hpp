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


#ifndef LAUNCH_CPP__ACTIONS__EXECUTE_PROCESS_HPP_
#define LAUNCH_CPP__ACTIONS__EXECUTE_PROCESS_HPP_

#include "launch_cpp/action.hpp"
#include "launch_cpp/substitution.hpp"
#include "launch_cpp/types.hpp"
#include "launch_cpp/safety/osal.hpp"
#include "launch_cpp/safety/command_builder.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

namespace launch_cpp
{

// Forward declaration
class Process;

class ExecuteProcess final : public Action
{
 public:
  struct Options
  {
    std::vector<SubstitutionPtr> cmd;
    SubstitutionPtr cwd;
    std::unordered_map<std::string, SubstitutionPtr> env;
    std::string output;  // "screen", "log", "both"
    bool emulateTty = false;
    std::int32_t sigtermTimeout = 5;
    SubstitutionPtr name;
    
    // Safety-related options
    bool enableSafety = false;
    std::uint64_t maxMemoryBytes = 0;  // 0 = unlimited
    double maxCpuPercent = 0.0;        // 0.0 = unlimited
    std::int32_t watchdogTimeoutMs = 0; // 0 = disabled
    
    // Retry policy options
    std::uint32_t maxRetries = 0;                    // 0 = no retry
    std::chrono::milliseconds retryDelay{5000};     // Delay between retries
    double retryBackoffMultiplier = 1.0;              // 1.0 = linear, >1.0 = exponential
    
    // Dependency options
    std::vector<std::string> dependsOn;              // List of process names this process depends on
  };
  
  explicit ExecuteProcess(const Options& options);
  
  ~ExecuteProcess() override;
  
  Result<void> Execute(LaunchContext& context) override;
  
  // Process control
  Error Shutdown();
  Error Terminate();
  Error Kill();
  void SendSignal(std::int32_t signal);

  // Status queries
  bool IsRunning() const noexcept;
  Result<std::int32_t> GetReturnCode() const;
  Result<std::int32_t> GetPid() const;
  std::string GetName() const;

  // Safety-related methods
  void SetProcessExecutor(std::shared_ptr<launch_cpp::ProcessExecutor> executor);
  void SetResourceMonitor(std::shared_ptr<launch_cpp::ResourceMonitor> monitor);
  void SetWatchdog(std::shared_ptr<launch_cpp::Watchdog> watchdog);
  bool CheckResourcesAvailable(std::uint64_t estimatedMemory) const;

 private:
  Options options_;
  std::unique_ptr<Process> process_;
  std::string resolvedName_;

  // Safety-related members
  std::shared_ptr<launch_cpp::ProcessExecutor> processExecutor_;
  std::shared_ptr<launch_cpp::ResourceMonitor> resourceMonitor_;
  std::shared_ptr<launch_cpp::Watchdog> watchdog_;
  launch_cpp::ProcessId processId_;

  // Convert Substitutions to command line
  std::vector<std::string> ResolveCommand(LaunchContext& context) const;

  // Validate and escape command line arguments
  Result<std::vector<std::string>> ValidateAndEscapeCommand(
      const std::vector<std::string>& cmd) const;

  // Retry logic implementation
  Result<void> ExecuteSingleAttempt(LaunchContext& context,
                                     const std::vector<std::string>& cmd);
  Result<void> ExecuteWithRetry(LaunchContext& context,
                                 const std::vector<std::string>& cmd);
  bool IsRetryableError(ErrorCode code) const;
  std::chrono::milliseconds CalculateRetryDelay(uint32_t attemptNumber) const;
  void CleanupBeforeRetry();
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__ACTIONS__EXECUTE_PROCESS_HPP_
