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


#include "launch_cpp/substitutions/launch_configuration.hpp"
#include "launch_cpp/launch_context.hpp"

namespace launch_cpp
{

std::string LaunchConfiguration::perform(const LaunchContext& context) const
{
  Result<std::string> result = context.get_launch_configuration(name_);
  
  if (result.has_error())
  {
    return "";
  }
  
  return result.get_value();
}

}  // namespace launch_cpp
