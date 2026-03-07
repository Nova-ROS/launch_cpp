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


#include "cpp_launch/action.hpp"
#include "cpp_launch/launch_context.hpp"
#include "cpp_launch/condition.hpp"

namespace cpp_launch
{

Result<LaunchDescriptionEntityVector> Action::Visit(LaunchContext& context)
{
  if (HasCondition())
  {
    bool conditionResult = GetCondition()->Evaluate(context);
    
    if (!conditionResult)
    {
      return Result<LaunchDescriptionEntityVector>(LaunchDescriptionEntityVector{});
    }
  }
  
  Result<void> executeResult = Execute(context);
  
  if (executeResult.HasError())
  {
    Result<LaunchDescriptionEntityVector> errorResult(executeResult.GetError());
    return errorResult;
  }
  
  return Result<LaunchDescriptionEntityVector>(LaunchDescriptionEntityVector{});
}

}  // namespace cpp_launch
