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

#include "cpp_launch/substitutions/this_launch_file_dir.hpp"
#include "cpp_launch/launch_context.hpp"
#include <string>

namespace cpp_launch
{

std::string ThisLaunchFileDir::Perform(const LaunchContext& context) const
{
  std::string launch_file = context.GetCurrentLaunchFile();
  if (launch_file.empty())
  {
    return "";
  }
  
  size_t last_slash = launch_file.find_last_of("/");
  if (last_slash == std::string::npos)
  {
    return ".";
  }
  
  return launch_file.substr(0, last_slash);
}

}  // namespace cpp_launch
