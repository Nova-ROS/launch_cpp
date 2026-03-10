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


#ifndef LAUNCH_CPP__SUBSTITUTIONS__ENVIRONMENT_VARIABLE_HPP_
#define LAUNCH_CPP__SUBSTITUTIONS__ENVIRONMENT_VARIABLE_HPP_

#include "launch_cpp/substitution.hpp"
#include <string>

namespace launch_cpp
{

class EnvironmentVariable final : public Substitution
{
 public:
  EnvironmentVariable(const std::string& name, const std::string& defaultValue = "")
    : name_(name), defaultValue_(defaultValue) {}
  
  ~EnvironmentVariable() override = default;
  
  std::string perform(const LaunchContext& context) const override;
  
  const std::string& GetName() const noexcept { return name_; }
  const std::string& GetDefaultValue() const noexcept { return defaultValue_; }
  
 private:
  std::string name_;
  std::string defaultValue_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__SUBSTITUTIONS__ENVIRONMENT_VARIABLE_HPP_
