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

#ifndef CPP_LAUNCH__ACTIONS__EXECUTE_PROCESS_HPP_
#define CPP_LAUNCH__ACTIONS__EXECUTE_PROCESS_HPP_

#include "cpp_launch/action.hpp"
#include "cpp_launch/substitution.hpp"
#include "cpp_launch/types.hpp"
#include <vector>
#include <unordered_map>
#include <memory>

namespace cpp_launch
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
  
 private:
  Options options_;
  std::unique_ptr<Process> process_;
  std::string resolvedName_;
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__ACTIONS__EXECUTE_PROCESS_HPP_
