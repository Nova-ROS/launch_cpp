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


#include "launch_cpp/substitutions/find_executable.hpp"
#include "launch_cpp/launch_context.hpp"
#include <cstdlib>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace launch_cpp
{

std::string FindExecutable::perform(const LaunchContext& context) const
{
  (void)context;  // Context not used, but required by interface

  const char* pathEnv = std::getenv("PATH");
  if (!pathEnv)
  {
    return name_;  // Return original name if PATH not set
  }

  std::string pathStr(pathEnv);
  std::istringstream iss(pathStr);
  std::string path;

  while (std::getline(iss, path, ':'))
  {
    std::string fullPath = path + "/" + name_;

    // Check if file exists and is executable
    struct stat st;
    if (stat(fullPath.c_str(), &st) == 0)
    {
      if (S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR))
      {
        return fullPath;
      }
    }
  }

  // Not found in PATH, return original name
  return name_;
}

}  // namespace launch_cpp
