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

#ifndef CPP_LAUNCH__ACTIONS__DECLARE_LAUNCH_ARGUMENT_HPP_
#define CPP_LAUNCH__ACTIONS__DECLARE_LAUNCH_ARGUMENT_HPP_

#include "cpp_launch/action.hpp"
#include "cpp_launch/substitution.hpp"
#include <string>
#include <vector>

namespace cpp_launch
{

class DeclareLaunchArgument final : public Action
{
 public:
  struct Options
  {
    std::string name;
    SubstitutionPtr defaultValue;
    std::string description;
    std::vector<std::string> choices;
  };
  
  explicit DeclareLaunchArgument(const Options& options);
  
  ~DeclareLaunchArgument() override = default;
  
  Result<void> Execute(LaunchContext& context) override;
  
  const std::string& GetName() const noexcept { return options_.name; }
  const SubstitutionPtr& GetDefaultValue() const noexcept { return options_.defaultValue; }
  const std::string& GetDescription() const noexcept { return options_.description; }
  const std::vector<std::string>& GetChoices() const noexcept { return options_.choices; }
  
 private:
  Options options_;
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__ACTIONS__DECLARE_LAUNCH_ARGUMENT_HPP_
