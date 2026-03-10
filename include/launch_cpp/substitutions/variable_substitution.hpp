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

/**
 * @file variable_substitution.hpp
 * @brief Variable substitution for launch files $(var name)
 * 
 * @details Implements variable substitution using $(var variable_name) syntax.
 * Variables are resolved from launch configurations.
 * 
 * @ASIL ASIL B
 * 
 * @requirements
 * - REQ-LAUNCH-SUBST-001: Support variable substitution
 * - REQ-LAUNCH-SUBST-002: Resolve variables from launch context
 */

#ifndef LAUNCH_CPP__SUBSTITUTIONS__VARIABLE_SUBSTITUTION_HPP_
#define LAUNCH_CPP__SUBSTITUTIONS__VARIABLE_SUBSTITUTION_HPP_

#include "launch_cpp/substitution.hpp"
#include "launch_cpp/launch_context.hpp"
#include <string>

namespace launch_cpp
{

/**
 * @brief Variable substitution using $(var name) syntax
 * 
 * @details Replaces $(var variable_name) with the value from launch configuration.
 * If the variable is not found, returns an empty string or the default value.
 * 
 * Example:
 * @code
 * VariableSubstitution var("my_param");
 * std::string result = var.Perform(context);
 * // If context has "my_param" = "value", result = "value"
 * @endcode
 * 
 * @requirement REQ-LAUNCH-SUBST-001
 */
class VariableSubstitution final : public Substitution
{
 public:
  /**
   * @brief Constructor
   * @param variable_name Name of the variable to substitute
   * @param default_value Default value if variable not found (optional)
   */
  explicit VariableSubstitution(
    const std::string& variable_name,
    const std::string& default_value = "")
    : variable_name_(variable_name), default_value_(default_value) {}

  /**
   * @brief Perform variable substitution
   * @param context Launch context containing variable values
   * @return Resolved variable value or default
   * 
   * @requirement REQ-LAUNCH-SUBST-002
   */
  std::string perform(const LaunchContext& context) const override
  {
    auto result = context.GetLaunchConfiguration(variable_name_);
    if (result.has_value())
    {
      return result.get_value();
    }
    return default_value_;
  }

  /**
   * @brief Get the variable name
   */
  const std::string& GetVariableName() const { return variable_name_; }

  /**
   * @brief Get the default value
   */
  const std::string& GetDefaultValue() const { return default_value_; }

 private:
  std::string variable_name_;
  std::string default_value_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__SUBSTITUTIONS__VARIABLE_SUBSTITUTION_HPP_
