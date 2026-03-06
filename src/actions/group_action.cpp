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

#include "cpp_launch/actions/group_action.hpp"
#include "cpp_launch/launch_context.hpp"
#include "cpp_launch/condition.hpp"

namespace cpp_launch
{

GroupAction::GroupAction(const Options& options)
  : Action(), options_(options)
{
}

Result<void> GroupAction::Execute(LaunchContext& context)
{
  if (options_.condition)
  {
    bool should_execute = options_.condition->Evaluate(context);
    if (!should_execute)
    {
      return Result<void>();
    }
  }

  for (const auto& action : options_.actions)
  {
    if (action)
    {
      Result<void> result = action->Execute(context);
      if (result.HasError())
      {
        return result;
      }
    }
  }

  return Result<void>();
}

}  // namespace cpp_launch
