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
    bool emulate_tty = false;
    std::int32_t sigterm_timeout = 5;
    SubstitutionPtr name;

    // Safety-related options
    bool enable_safety = false;
    std::uint64_t max_memory_bytes = 0;  // 0 = unlimited
    double max_cpu_percent = 0.0;        // 0.0 = unlimited
    std::int32_t watchdog_timeout_ms = 0; // 0 = disabled

    // Retry policy options
    std::uint32_t max_retries = 0;                    // 0 = no retry
    std::chrono::milliseconds retry_delay{5000};     // Delay between retries
    double retry_backoff_multiplier = 1.0;              // 1.0 = linear, >1.0 = exponential

    // Dependency options
    std::vector<std::string> depends_on;              // List of process names this process depends on
  };

  explicit ExecuteProcess(const Options& options);

  ~ExecuteProcess() override;

  Result<void> execute(LaunchContext& context) override;

  // Process control
  Error shutdown();
  Error terminate();
  Error kill();
  void send_signal(std::int32_t signal);

  // Status queries
  bool is_running() const noexcept;
  Result<std::int32_t> get_return_code() const;
  Result<std::int32_t> get_pid() const;
  std::string get_name() const;

  // Safety-related methods
  void set_process_executor(std::shared_ptr<launch_cpp::ProcessExecutor> executor);
  void set_resource_monitor(std::shared_ptr<launch_cpp::ResourceMonitor> monitor);
  void set_watchdog(std::shared_ptr<launch_cpp::Watchdog> watchdog);
  bool check_resources_available(std::uint64_t estimated_memory) const;

 private:
  Options options_;
  std::unique_ptr<Process> process_;
  std::string resolved_name_;

  // Safety-related members
  std::shared_ptr<launch_cpp::ProcessExecutor> process_executor_;
  std::shared_ptr<launch_cpp::ResourceMonitor> resource_monitor_;
  std::shared_ptr<launch_cpp::Watchdog> watchdog_;
  launch_cpp::ProcessId process_id_;

  // Convert Substitutions to command line
  std::vector<std::string> resolve_command(LaunchContext& context) const;

  // Validate and escape command line arguments
  Result<std::vector<std::string>> validate_and_escape_command(
      const std::vector<std::string>& cmd) const;

  // Retry logic implementation
  Result<void> execute_single_attempt(LaunchContext& context,
                                     const std::vector<std::string>& cmd);
  Result<void> execute_with_retry(LaunchContext& context,
                                 const std::vector<std::string>& cmd);
  bool is_retryable_error(ErrorCode code) const;
  std::chrono::milliseconds calculate_retry_delay(uint32_t attempt_number) const;
  void cleanup_before_retry();
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__ACTIONS__EXECUTE_PROCESS_HPP_
