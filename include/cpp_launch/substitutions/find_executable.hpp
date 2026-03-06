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

#ifndef CPP_LAUNCH__SUBSTITUTIONS__FIND_EXECUTABLE_HPP_
#define CPP_LAUNCH__SUBSTITUTIONS__FIND_EXECUTABLE_HPP_

#include "cpp_launch/substitution.hpp"
#include <string>

namespace cpp_launch
{

// FindExecutable substitution - finds executable in PATH
class FindExecutable final : public Substitution
{
 public:
  explicit FindExecutable(std::string name)
    : name_(std::move(name)) {}
  
  ~FindExecutable() override = default;
  
  std::string Perform(const LaunchContext& context) const override;
  
 private:
  std::string name_;
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__SUBSTITUTIONS__FIND_EXECUTABLE_HPP_
