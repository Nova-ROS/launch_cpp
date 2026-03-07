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


#ifndef CPP_LAUNCH__SUBSTITUTIONS__COMMAND_HPP_
#define CPP_LAUNCH__SUBSTITUTIONS__COMMAND_HPP_

#include "cpp_launch/substitution.hpp"
#include <vector>

namespace cpp_launch
{

// Command substitution - executes a command and returns its output
class Command final : public Substitution
{
 public:
  explicit Command(std::vector<SubstitutionPtr> command)
    : command_(std::move(command)) {}
  
  ~Command() override = default;
  
  std::string Perform(const LaunchContext& context) const override;
  
 private:
  std::vector<SubstitutionPtr> command_;
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__SUBSTITUTIONS__COMMAND_HPP_
