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


#ifndef LAUNCH_CPP__CONDITIONS__IF_CONDITION_HPP_
#define LAUNCH_CPP__CONDITIONS__IF_CONDITION_HPP_

#include "launch_cpp/condition.hpp"
#include "launch_cpp/substitution.hpp"

namespace launch_cpp
{

class IfCondition final : public Condition
{
 public:
  explicit IfCondition(const SubstitutionPtr& expression)
    : expression_(expression) {}
  
  ~IfCondition() override = default;
  
  bool evaluate(const LaunchContext& context) const override;
  
  const SubstitutionPtr& get_expression() const noexcept { return expression_; }
  
 private:
  SubstitutionPtr expression_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__CONDITIONS__IF_CONDITION_HPP_
