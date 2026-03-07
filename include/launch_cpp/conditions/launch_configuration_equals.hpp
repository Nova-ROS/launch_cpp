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


#ifndef LAUNCH_CPP__CONDITIONS__LAUNCH_CONFIGURATION_EQUALS_HPP_
#define LAUNCH_CPP__CONDITIONS__LAUNCH_CONFIGURATION_EQUALS_HPP_

#include "launch_cpp/condition.hpp"
#include "launch_cpp/substitution.hpp"
#include <string>

namespace launch_cpp
{

class LaunchConfigurationEquals final : public Condition
{
 public:
  LaunchConfigurationEquals(const std::string& name, const SubstitutionPtr& expected)
    : name_(name), expected_(expected) {}
  
  ~LaunchConfigurationEquals() override = default;
  
  bool Evaluate(const LaunchContext& context) const override;
  
  const std::string& GetName() const noexcept { return name_; }
  const SubstitutionPtr& GetExpected() const noexcept { return expected_; }
  
 private:
  std::string name_;
  SubstitutionPtr expected_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__CONDITIONS__LAUNCH_CONFIGURATION_EQUALS_HPP_
