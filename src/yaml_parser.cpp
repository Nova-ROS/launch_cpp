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


#include "launch_cpp/yaml_parser.hpp"
#include "launch_cpp/actions/execute_process.hpp"
#include "launch_cpp/actions/declare_launch_argument.hpp"
#include "launch_cpp/substitutions/text_substitution.hpp"
#include "launch_cpp/substitutions/launch_configuration.hpp"
#include "launch_cpp/substitutions/variable_substitution.hpp"
#include "launch_cpp/substitutions/environment_variable.hpp"
#include "launch_cpp/conditions/if_condition.hpp"
#include <cctype>
#include <algorithm>

namespace launch_cpp
{

Result<YamlValue> YamlParser::parse(const std::string& content)
{
  std::istringstream stream(content);
  int line = 0;
  return parse_object(stream, line, 0);
}

Result<YamlValue> YamlParser::parse_file(const std::string& file_path)
{
  std::ifstream file(file_path);
  if (!file.is_open())
  {
    return Result<YamlValue>(Error(ErrorCode::K_INVALID_ARGUMENT, "Failed to open file: " + file_path));
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
  file.close();

  return parse(content);
}

Result<YamlValue> YamlParser::parse_value(std::istringstream& stream, int& line)
{
  std::string line_str;
  if (!std::getline(stream, line_str))
  {
    return Result<YamlValue>(YamlValue());
  }

  line++;
  line_str = trim(line_str);

  if (line_str.empty() || line_str[0] == '#')
  {
    return parse_value(stream, line);
  }

  return parse_scalar(line_str);
}

Result<YamlValue> YamlParser::parse_scalar(const std::string& value)
{
  std::string trimmed = trim(value);

  // Check for boolean
  if (trimmed == "true" || trimmed == "yes" || trimmed == "on")
  {
    return Result<YamlValue>(YamlValue(true));
  }
  if (trimmed == "false" || trimmed == "no" || trimmed == "off")
  {
    return Result<YamlValue>(YamlValue(false));
  }

  // Check for number
  bool is_number = true;
  bool has_dot = false;
  for (size_t i = 0; i < trimmed.size(); ++i)
  {
    if (i == 0 && (trimmed[i] == '-' || trimmed[i] == '+')) continue;
    if (trimmed[i] == '.')
    {
      if (has_dot)
      {
        is_number = false;
        break;
      }
      has_dot = true;
      continue;
    }
    if (!std::isdigit(static_cast<unsigned char>(trimmed[i])))
    {
      is_number = false;
      break;
    }
  }

  if (is_number && !trimmed.empty())
  {
    try
    {
      double num = std::stod(trimmed);
      return Result<YamlValue>(YamlValue(num));
    }
    catch (...)
    {
      // Fall through to string
    }
  }

  // String value
  return Result<YamlValue>(YamlValue(unquote(trimmed)));
}

Result<YamlValue> YamlParser::parse_array(std::istringstream& stream, int& line, int base_indent)
{
  YamlValue result;
  std::string line_str;

  while (std::getline(stream, line_str))
  {
    line++;

    std::string trimmed_line = trim(line_str);
    if (trimmed_line.empty() || trimmed_line[0] == '#')
    {
      continue;
    }

    int indent = get_indent(line_str);
    if (indent < base_indent)
    {
      // End of array - put line back
      stream.seekg(-static_cast<int>(line_str.length()) - 1, std::ios::cur);
      line--;
      break;
    }

    if (trimmed_line[0] == '-')
    {
      // Remove the '-'
      std::string content = trim(trimmed_line.substr(1));

      if (content.empty())
      {
        // Multi-line value or object starts on next line
        auto next_result = parse_value(stream, line);
        if (next_result.has_error())
        {
          return next_result;
        }
        result.add_array_element(next_result.get_value());
      } else if (content.find(':') != std::string::npos) {
        // This is an inline object start like "- type: execute_process"
        // Parse this and subsequent lines at higher indent
        YamlValue element = parse_array_element_object(stream, line, line_str, base_indent);
        result.add_array_element(element);
      } else {
        // Simple scalar value
        auto scalar_result = parse_scalar(content);
        if (scalar_result.has_error())
        {
          return scalar_result;
        }
        result.add_array_element(scalar_result.get_value());
      }
    }
  }

  return Result<YamlValue>(result);
}

// Parse an object element in an array (handles "- key: value" and subsequent fields)
YamlValue launch_cpp::YamlParser::parse_array_element_object(std::istringstream& stream, int& line,
                                               const std::string& first_line, int base_indent)
{
  YamlValue object_value;

  // Parse the first line (e.g., "- type: execute_process")
  std::string trimmed_first = trim(first_line);
  std::string content = trim(trimmed_first.substr(1));  // Remove '-'

  size_t colon_pos = content.find(':');
  if (colon_pos != std::string::npos)
  {
    std::string key = trim(content.substr(0, colon_pos));
    std::string value = trim(content.substr(colon_pos + 1));

    if (value.empty())
    {
      // Value is on subsequent lines - check if it's an array or object/scalar
      auto pos = stream.tellg();
      std::string next_line;
      if (std::getline(stream, next_line))
      {
        line++;
        std::string trimmed_next = trim(next_line);
        int next_indent = get_indent(next_line);

        if (next_indent > base_indent && !trimmed_next.empty() && trimmed_next[0] == '-')
        {
          // It's an array - parse it
          stream.seekg(pos);
          line--;
          auto array_result = parse_array(stream, line, next_indent);
          if (!array_result.has_error())
          {
            object_value.set_object_field(key, array_result.get_value());
          }
        } else {
          // It's a nested object or scalar
          stream.seekg(pos);
          line--;
          auto nested_result = parse_value(stream, line);
          if (!nested_result.has_error())
          {
            object_value.set_object_field(key, nested_result.get_value());
          }
        }
      } else {
        // No more lines - empty value
        object_value.set_object_field(key, YamlValue());
      }
    } else {
      // Inline value
      auto scalar_result = parse_scalar(value);
      if (!scalar_result.has_error())
      {
        object_value.set_object_field(key, scalar_result.get_value());
      }
    }
  }

  // Continue reading additional fields at higher indent level
  std::string line_str;
  auto pos = stream.tellg();

  while (std::getline(stream, line_str))
  {
    line++;

    std::string trimmed = trim(line_str);
    if (trimmed.empty() || trimmed[0] == '#')
    {
      pos = stream.tellg();
      continue;
    }

    int indent = get_indent(line_str);
    if (indent <= base_indent)
    {
      // End of this object element
      stream.seekg(pos);
      line--;
      break;
    }

    // This is a field of our object - parse it
    // Remove the leading whitespace to make it look like a top-level line
    std::string deindented = line_str.substr(base_indent + 2);  // +2 for the "- " we removed
    std::istringstream field_stream(deindented);
    int field_line = 0;

    // Temporarily replace stream position to parse just this field
    auto saved_pos = stream.tellg();

    // Check if it's a key-value pair
    size_t colon_pos = trimmed.find(':');
    if (colon_pos != std::string::npos && trimmed[0] != '-')
    {
      std::string key = trim(trimmed.substr(0, colon_pos));
      std::string value = trim(trimmed.substr(colon_pos + 1));

      if (value.empty())
      {
        // Multi-line value - check if it's array, object, or scalar
        auto after_key_pos = stream.tellg();
        std::string next_line2;
        if (std::getline(stream, next_line2))
        {
          line++;
          std::string trimmed_next2 = trim(next_line2);
          int next_indent2 = get_indent(next_line2);

          if (next_indent2 > indent && !trimmed_next2.empty() && trimmed_next2[0] == '-')
          {
            // It's an array
            stream.seekg(after_key_pos);
            line--;
            auto array_result = parse_array(stream, line, next_indent2);
            if (!array_result.has_error())
            {
              object_value.set_object_field(key, array_result.get_value());
            }
          } else {
            // It's a nested object or scalar
            stream.seekg(after_key_pos);
            line--;
            auto value_result = parse_value(stream, line);
            if (!value_result.has_error())
            {
              object_value.set_object_field(key, value_result.get_value());
            }
          }
        } else {
          // No more lines - empty value
          object_value.set_object_field(key, YamlValue());
        }
      } else {
        // Inline value
        auto scalar_result = parse_scalar(value);
        if (!scalar_result.has_error())
        {
          object_value.set_object_field(key, scalar_result.get_value());
        }
      }
    } else if (trimmed[0] == '-') {
      // This is an array element at a nested level
      // Need special handling - back up and parse as array
      stream.seekg(pos);
      line--;
      auto array_result = parse_array(stream, line, indent);
      if (!array_result.has_error())
      {
        // This array becomes the value of the previous key
        // But we don't have the key here... this is getting complex
        // For now, store it with a special marker
        // Actually, this shouldn't happen in well-formed YAML for our use case
      }
      break;
    }

    pos = stream.tellg();
  }

  return object_value;
}

Result<YamlValue> YamlParser::parse_object(std::istringstream& stream, int& line, int base_indent)
{
  YamlValue result;
  std::string line_str;
  std::string current_key;

  while (std::getline(stream, line_str))
  {
    line++;

    std::string trimmed = trim(line_str);
    if (trimmed.empty() || trimmed[0] == '#')
    {
      continue;
    }

    int indent = get_indent(line_str);
    if (indent < base_indent && base_indent > 0)
    {
      // End of this object
      break;
    }

    // Parse key-value pair
    size_t colon_pos = trimmed.find(':');
    if (colon_pos == std::string::npos)
    {
      continue;  // Skip invalid lines
    }

    std::string key = trim(trimmed.substr(0, colon_pos));
    std::string value = trim(trimmed.substr(colon_pos + 1));

    if (value.empty())
    {
      // Check next line for value
      auto pos = stream.tellg();
      std::string next_line;
      bool found_next = false;

      // Skip empty lines and comments to find the real next line
      while (std::getline(stream, next_line))
      {
        std::string trimmed_next = trim(next_line);
        if (!trimmed_next.empty() && trimmed_next[0] != '#')
        {
          found_next = true;
          break;
        }
      }

      if (found_next)
      {
        int next_indent = get_indent(next_line);
        stream.seekg(pos);

        if (next_indent > indent)
        {
          // Nested object or array
          if (trim(next_line)[0] == '-')
          {
            auto array_result = parse_array(stream, line, next_indent);
            if (array_result.has_error())
            {
              return array_result;
            }
            result.set_object_field(key, array_result.get_value());
          } else {
            auto obj_result = parse_object(stream, line, next_indent);
            if (obj_result.has_error())
            {
              return obj_result;
            }
            result.set_object_field(key, obj_result.get_value());
          }
        } else {
          result.set_object_field(key, YamlValue());
        }
      } else {
        result.set_object_field(key, YamlValue());
      }
    } else if (value[0] == '[') {
      // Inline array
      YamlValue arr;
      size_t start = 1;
      while (start < value.size())
      {
        size_t end = value.find(',', start);
        if (end == std::string::npos)
        {
          end = value.find(']', start);
        }
        if (end == std::string::npos) break;

        std::string elem = trim(value.substr(start, end - start));
        if (!elem.empty())
        {
          auto elem_result = parse_scalar(elem);
          if (elem_result.has_value())
          {
            arr.add_array_element(elem_result.get_value());
          }
        }
        start = end + 1;
      }
      result.set_object_field(key, arr);
    } else {
      // Simple value
      auto val_result = parse_scalar(value);
      if (val_result.has_error())
      {
        return val_result;
      }
      result.set_object_field(key, val_result.get_value());
    }
  }

  return Result<YamlValue>(result);
}

std::string YamlParser::trim(const std::string& str)
{
  size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

int YamlParser::get_indent(const std::string& line)
{
  int indent = 0;
  for (char c : line) {
    if (c == ' ') {
      indent++;
    } else if (c == '\t') {
      indent += 2;
    } else {
      break;
    }
  }
  return indent;
}

std::string YamlParser::unquote(const std::string& str)
{
  if (str.size() >= 2)
  {
    if ((str[0] == '"' && str[str.size() - 1] == '"') ||
        (str[0] == '\'' && str[str.size() - 1] == '\''))
    {
      return str.substr(1, str.size() - 2);
    }
  }
  return str;
}

// Build launch description from YAML
Result<LaunchDescriptionPtr> YamlLaunchBuilder::build(const YamlValue& yaml)
{
  if (!yaml.is_object())
  {
    return Result<LaunchDescriptionPtr>(Error(ErrorCode::K_INVALID_CONFIGURATION, "Root must be an object"));
  }

  auto desc = std::make_shared<LaunchDescription>();

  auto entities = yaml.as_object().find("entities");
  if (entities == yaml.as_object().end() || !entities->second.is_array())
  {
    return Result<LaunchDescriptionPtr>(Error(ErrorCode::K_INVALID_CONFIGURATION, "Missing 'entities' array"));
  }

  for (const auto& entity_yaml : entities->second.as_array())
  {
    if (!entity_yaml.is_object())
    {
      continue;
    }

    auto action_result = build_action(entity_yaml);
    if (action_result.has_error())
    {
      continue;
    }

    desc->add(action_result.get_value());
  }

  return Result<LaunchDescriptionPtr>(desc);
}

Result<ActionPtr> YamlLaunchBuilder::build_action(const YamlValue& action_yaml)
{
  auto type_it = action_yaml.as_object().find("type");
  if (type_it == action_yaml.as_object().end() || !type_it->second.is_string())
  {
    return Result<ActionPtr>(Error(ErrorCode::K_INVALID_CONFIGURATION, "Action missing 'type' field"));
  }

  std::string type = type_it->second.as_string();

  if (type == "execute_process")
  {
    ExecuteProcess::Options options;

    // Parse process name
    auto name_it = action_yaml.as_object().find("name");
    if (name_it != action_yaml.as_object().end() && name_it->second.is_string())
    {
      options.name = std::make_shared<TextSubstitution>(name_it->second.as_string());
    }

    // Parse command with variable substitution support
    auto cmd_it = action_yaml.as_object().find("cmd");
    if (cmd_it != action_yaml.as_object().end() && cmd_it->second.is_array())
    {
      for (const auto& cmd_elem : cmd_it->second.as_array())
      {
        if (cmd_elem.is_string())
        {
          auto subst_result = build_substitution(cmd_elem.as_string());
          if (subst_result.has_value())
          {
            options.cmd.push_back(subst_result.get_value());
          } else {
            // Fallback to text if substitution building fails
            options.cmd.push_back(std::make_shared<TextSubstitution>(cmd_elem.as_string()));
          }
        }
      }
    }

    // Parse output
    auto output_it = action_yaml.as_object().find("output");
    if (output_it != action_yaml.as_object().end() && output_it->second.is_string())
    {
      options.output = output_it->second.as_string();
    }

    // Parse dependencies
    auto depends_it = action_yaml.as_object().find("depends_on");
    if (depends_it != action_yaml.as_object().end() && depends_it->second.is_array())
    {
      for (const auto& dep_elem : depends_it->second.as_array())
      {
        if (dep_elem.is_string())
        {
          options.depends_on.push_back(dep_elem.as_string());
        }
      }
    }

    return Result<ActionPtr>(std::make_shared<ExecuteProcess>(options));
  } else if (type == "declare_launch_argument") {
    DeclareLaunchArgument::Options options;

    auto name_it = action_yaml.as_object().find("name");
    if (name_it != action_yaml.as_object().end() && name_it->second.is_string())
    {
      options.name = name_it->second.as_string();
    }

    auto default_it = action_yaml.as_object().find("default_value");
    if (default_it != action_yaml.as_object().end() && default_it->second.is_string())
    {
      options.defaultValue = std::make_shared<TextSubstitution>(default_it->second.as_string());
    }

    auto desc_it = action_yaml.as_object().find("description");
    if (desc_it != action_yaml.as_object().end() && desc_it->second.is_string())
    {
      options.description = desc_it->second.as_string();
    }

    return Result<ActionPtr>(std::make_shared<DeclareLaunchArgument>(options));
  }

  return Result<ActionPtr>(Error(ErrorCode::K_NOT_IMPLEMENTED, "Unknown action type: " + type));
}

Result<SubstitutionPtr> YamlLaunchBuilder::build_substitution(const std::string& value)
{
  // Local trim function
  auto local_trim = [](const std::string& s) -> std::string {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
  };

  // Check for variable substitution syntax: $(var variable_name)
  if (value.length() > 7 && value.substr(0, 6) == "$(var ")
  {
    size_t end = value.find(')', 6);
    if (end != std::string::npos)
    {
      std::string var_name = value.substr(6, end - 6);
      // Trim whitespace
      var_name = local_trim(var_name);
      if (!var_name.empty())
      {
        return Result<SubstitutionPtr>(std::make_shared<VariableSubstitution>(var_name));
      }
    }
  }

  // Check for launch configuration syntax: $(find pkg) or other substitutions
  if (value.length() > 3 && value[0] == '$' && value[1] == '(')
  {
    size_t space_pos = value.find(' ', 2);
    size_t end_pos = value.find(')', 2);

    if (end_pos != std::string::npos)
    {
      std::string subst_type = value.substr(2, space_pos - 2);

      if (subst_type == "find" && space_pos != std::string::npos)
      {
        // $(find package_name) - return as text for now
        // TODO(launch_cpp): Implement find_package substitution
        return Result<SubstitutionPtr>(std::make_shared<TextSubstitution>(value));
      } else if (subst_type == "env" && space_pos != std::string::npos) {
        // $(env VAR_NAME) - environment variable
        std::string var_name = value.substr(space_pos + 1, end_pos - space_pos - 1);
        var_name = local_trim(var_name);
        return Result<SubstitutionPtr>(std::make_shared<EnvironmentVariable>(var_name));
      }
      // Add more substitution types as needed
    }
  }

  // Default: treat as text
  return Result<SubstitutionPtr>(std::make_shared<TextSubstitution>(value));
}

Result<ConditionPtr> YamlLaunchBuilder::build_condition(const YamlValue& condition_yaml)
{
  (void)condition_yaml;
  // TODO(launch_cpp): Implement condition building
  return Result<ConditionPtr>(Error(ErrorCode::K_NOT_IMPLEMENTED, "Condition building not yet implemented"));
}

}  // namespace launch_cpp
