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

#include "cpp_launch/conditions/if_condition.hpp"
#include "cpp_launch/launch_context.hpp"
#include "cpp_launch/substitution.hpp"

namespace cpp_launch
{

bool IfCondition::Evaluate(const LaunchContext& context) const
{
  if (!expression_)
  {
    return false;
  }
  
  std::string result = expression_->Perform(context);
  
  // Truthy check: not empty and not "false" or "0"
  if (result.empty())
  {
    return false;
  }
  
  if (result == "false" || result == "0")
  {
    return false;
  }
  
  return true;
}

}  // namespace cpp_launch
