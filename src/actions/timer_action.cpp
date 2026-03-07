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


#include "cpp_launch/actions/timer_action.hpp"
#include "cpp_launch/launch_context.hpp"
#include <thread>

namespace cpp_launch
{

TimerAction::TimerAction(const Options& options)
  : Action(), options_(options)
{
}

Result<void> TimerAction::Execute(LaunchContext& context)
{
  std::this_thread::sleep_for(options_.period);

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
