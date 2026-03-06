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

#ifndef CPP_LAUNCH__SUBSTITUTION_HPP_
#define CPP_LAUNCH__SUBSTITUTION_HPP_

// AUTOSAR C++14 Compliant Substitution Interface

#include <string>
#include "cpp_launch/types.hpp"

namespace cpp_launch
{

// Forward declaration
class LaunchContext;

// AUTOSAR C++14: A12-8-4 - Base class with virtual destructor
class Substitution
{
 public:
  Substitution() = default;
  
  // AUTOSAR C++14: A12-8-4 - Virtual destructor
  virtual ~Substitution() {}
  
  // AUTOSAR C++14: A10-3-3 - Declare special functions
  Substitution(const Substitution&) = default;
  Substitution& operator=(const Substitution&) = default;
  Substitution(Substitution&&) = default;
  Substitution& operator=(Substitution&&) = default;
  
  // AUTOSAR C++14: M0-1-9 - Pure virtual function
  virtual std::string Perform(const LaunchContext& context) const = 0;
  
  // Convenience operator
  std::string operator()(const LaunchContext& context) const
  {
    return Perform(context);
  }
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__SUBSTITUTION_HPP_
