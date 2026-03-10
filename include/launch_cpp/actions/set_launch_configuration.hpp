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


#ifndef LAUNCH_CPP__ACTIONS__SET_LAUNCH_CONFIGURATION_HPP_
#define LAUNCH_CPP__ACTIONS__SET_LAUNCH_CONFIGURATION_HPP_

#include "launch_cpp/action.hpp"
#include "launch_cpp/substitution.hpp"
#include <string>

namespace launch_cpp
{

class SetLaunchConfiguration final : public Action
{
 public:
  struct Options
  {
    std::string name;
    SubstitutionPtr value;
  };

  explicit SetLaunchConfiguration(const Options& options);

  ~SetLaunchConfiguration() override = default;

  Result<void> execute(LaunchContext& context) override;

  const std::string& get_name() const noexcept { return options_.name; }
  const SubstitutionPtr& get_value() const noexcept { return options_.value; }

 private:
  Options options_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__ACTIONS__SET_LAUNCH_CONFIGURATION_HPP_
