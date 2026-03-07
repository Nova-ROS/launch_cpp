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


#include "launch_cpp/substitutions/environment_variable.hpp"
#include "launch_cpp/launch_context.hpp"
#include <cstdlib>

namespace launch_cpp
{

std::string EnvironmentVariable::Perform(const LaunchContext& context) const
{
  (void)context;  // Not used, but required by interface
  
  const char* value = std::getenv(name_.c_str());
  
  if (value == nullptr)
  {
    return defaultValue_;
  }
  
  return std::string(value);
}

}  // namespace launch_cpp
