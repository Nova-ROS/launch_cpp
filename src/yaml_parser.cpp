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

Result<YamlValue> YamlParser::Parse(const std::string& content)
{
  std::istringstream stream(content);
  int line = 0;
  return ParseObject(stream, line, 0);
}

Result<YamlValue> YamlParser::ParseFile(const std::string& filePath)
{
  std::ifstream file(filePath);
  if (!file.is_open())
  {
    return Result<YamlValue>(Error(ErrorCode::kInvalidArgument, "Failed to open file: " + filePath));
  }
  
  std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
  file.close();
  
  return Parse(content);
}

Result<YamlValue> YamlParser::ParseValue(std::istringstream& stream, int& line)
{
  std::string lineStr;
  if (!std::getline(stream, lineStr))
  {
    return Result<YamlValue>(YamlValue());
  }
  
  line++;
  lineStr = Trim(lineStr);
  
  if (lineStr.empty() || lineStr[0] == '#')
  {
    return ParseValue(stream, line);
  }
  
  return ParseScalar(lineStr);
}

Result<YamlValue> YamlParser::ParseScalar(const std::string& value)
{
  std::string trimmed = Trim(value);
  
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
  bool isNumber = true;
  bool hasDot = false;
  for (size_t i = 0; i < trimmed.size(); ++i)
  {
    if (i == 0 && (trimmed[i] == '-' || trimmed[i] == '+')) continue;
    if (trimmed[i] == '.')
    {
      if (hasDot)
      {
        isNumber = false;
        break;
      }
      hasDot = true;
      continue;
    }
    if (!std::isdigit(static_cast<unsigned char>(trimmed[i])))
    {
      isNumber = false;
      break;
    }
  }
  
  if (isNumber && !trimmed.empty())
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
  return Result<YamlValue>(YamlValue(Unquote(trimmed)));
}

Result<YamlValue> YamlParser::ParseArray(std::istringstream& stream, int& line, int baseIndent)
{
  YamlValue result;
  std::string lineStr;
  
  while (std::getline(stream, lineStr))
  {
    line++;
    
    std::string trimmedLine = Trim(lineStr);
    if (trimmedLine.empty() || trimmedLine[0] == '#')
    {
      continue;
    }
    
    int indent = GetIndent(lineStr);
    if (indent < baseIndent)
    {
      // End of array - put line back
      stream.seekg(-static_cast<int>(lineStr.length()) - 1, std::ios::cur);
      line--;
      break;
    }
    
    if (trimmedLine[0] == '-')
    {
      // Remove the '-'
      std::string content = Trim(trimmedLine.substr(1));
      
      if (content.empty())
      {
        // Multi-line value or object starts on next line
        auto nextResult = ParseValue(stream, line);
        if (nextResult.HasError())
        {
          return nextResult;
        }
        result.AddArrayElement(nextResult.GetValue());
      }
      else if (content.find(':') != std::string::npos)
      {
        // This is an inline object start like "- type: execute_process"
        // Parse this and subsequent lines at higher indent
        YamlValue element = ParseArrayElementObject(stream, line, lineStr, baseIndent);
        result.AddArrayElement(element);
      }
      else
      {
        // Simple scalar value
        auto scalarResult = ParseScalar(content);
        if (scalarResult.HasError())
        {
          return scalarResult;
        }
        result.AddArrayElement(scalarResult.GetValue());
      }
    }
  }
  
  return Result<YamlValue>(result);
}

// Parse an object element in an array (handles "- key: value" and subsequent fields)
YamlValue launch_cpp::YamlParser::ParseArrayElementObject(std::istringstream& stream, int& line, 
                                               const std::string& firstLine, int baseIndent)
{
  YamlValue objectValue;
  
  // Parse the first line (e.g., "- type: execute_process")
  std::string trimmedFirst = Trim(firstLine);
  std::string content = Trim(trimmedFirst.substr(1)); // Remove '-'
  
  size_t colonPos = content.find(':');
  if (colonPos != std::string::npos)
  {
    std::string key = Trim(content.substr(0, colonPos));
    std::string value = Trim(content.substr(colonPos + 1));
    
    if (value.empty())
    {
      // Value is on subsequent lines - check if it's an array or object/scalar
      auto pos = stream.tellg();
      std::string nextLine;
      if (std::getline(stream, nextLine))
      {
        line++;
        std::string trimmedNext = Trim(nextLine);
        int nextIndent = GetIndent(nextLine);
        
        if (nextIndent > baseIndent && !trimmedNext.empty() && trimmedNext[0] == '-')
        {
          // It's an array - parse it
          stream.seekg(pos);
          line--;
          auto arrayResult = ParseArray(stream, line, nextIndent);
          if (!arrayResult.HasError())
          {
            objectValue.SetObjectField(key, arrayResult.GetValue());
          }
        }
        else
        {
          // It's a nested object or scalar
          stream.seekg(pos);
          line--;
          auto nestedResult = ParseValue(stream, line);
          if (!nestedResult.HasError())
          {
            objectValue.SetObjectField(key, nestedResult.GetValue());
          }
        }
      }
      else
      {
        // No more lines - empty value
        objectValue.SetObjectField(key, YamlValue());
      }
    }
    else
    {
      // Inline value
      auto scalarResult = ParseScalar(value);
      if (!scalarResult.HasError())
      {
        objectValue.SetObjectField(key, scalarResult.GetValue());
      }
    }
  }
  
  // Continue reading additional fields at higher indent level
  std::string lineStr;
  auto pos = stream.tellg();
  
  while (std::getline(stream, lineStr))
  {
    line++;
    
    std::string trimmed = Trim(lineStr);
    if (trimmed.empty() || trimmed[0] == '#')
    {
      pos = stream.tellg();
      continue;
    }
    
    int indent = GetIndent(lineStr);
    if (indent <= baseIndent)
    {
      // End of this object element
      stream.seekg(pos);
      line--;
      break;
    }
    
    // This is a field of our object - parse it
    // Remove the leading whitespace to make it look like a top-level line
    std::string deindented = lineStr.substr(baseIndent + 2); // +2 for the "- " we removed
    std::istringstream fieldStream(deindented);
    int fieldLine = 0;
    
    // Temporarily replace stream position to parse just this field
    auto savedPos = stream.tellg();
    
    // Check if it's a key-value pair
    size_t colonPos = trimmed.find(':');
    if (colonPos != std::string::npos && trimmed[0] != '-')
    {
      std::string key = Trim(trimmed.substr(0, colonPos));
      std::string value = Trim(trimmed.substr(colonPos + 1));
      
      if (value.empty())
      {
        // Multi-line value - check if it's array, object, or scalar
        auto afterKeyPos = stream.tellg();
        std::string nextLine2;
        if (std::getline(stream, nextLine2))
        {
          line++;
          std::string trimmedNext2 = Trim(nextLine2);
          int nextIndent2 = GetIndent(nextLine2);
          
          if (nextIndent2 > indent && !trimmedNext2.empty() && trimmedNext2[0] == '-')
          {
            // It's an array
            stream.seekg(afterKeyPos);
            line--;
            auto arrayResult = ParseArray(stream, line, nextIndent2);
            if (!arrayResult.HasError())
            {
              objectValue.SetObjectField(key, arrayResult.GetValue());
            }
          }
          else
          {
            // It's a nested object or scalar
            stream.seekg(afterKeyPos);
            line--;
            auto valueResult = ParseValue(stream, line);
            if (!valueResult.HasError())
            {
              objectValue.SetObjectField(key, valueResult.GetValue());
            }
          }
        }
        else
        {
          // No more lines - empty value
          objectValue.SetObjectField(key, YamlValue());
        }
      }
      else
      {
        // Inline value
        auto scalarResult = ParseScalar(value);
        if (!scalarResult.HasError())
        {
          objectValue.SetObjectField(key, scalarResult.GetValue());
        }
      }
    }
    else if (trimmed[0] == '-')
    {
      // This is an array element at a nested level
      // Need special handling - back up and parse as array
      stream.seekg(pos);
      line--;
      auto arrayResult = ParseArray(stream, line, indent);
      if (!arrayResult.HasError())
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
  
  return objectValue;
}

Result<YamlValue> YamlParser::ParseObject(std::istringstream& stream, int& line, int baseIndent)
{
  YamlValue result;
  std::string lineStr;
  std::string currentKey;
  
  while (std::getline(stream, lineStr))
  {
    line++;
    
    std::string trimmed = Trim(lineStr);
    if (trimmed.empty() || trimmed[0] == '#')
    {
      continue;
    }
    
    int indent = GetIndent(lineStr);
    if (indent < baseIndent && baseIndent > 0)
    {
      // End of this object
      break;
    }
    
    // Parse key-value pair
    size_t colonPos = trimmed.find(':');
    if (colonPos == std::string::npos)
    {
      continue;  // Skip invalid lines
    }
    
    std::string key = Trim(trimmed.substr(0, colonPos));
    std::string value = Trim(trimmed.substr(colonPos + 1));
    
    if (value.empty())
    {
      // Check next line for value
      auto pos = stream.tellg();
      std::string nextLine;
      bool foundNext = false;
      
      // Skip empty lines and comments to find the real next line
      while (std::getline(stream, nextLine))
      {
        std::string trimmedNext = Trim(nextLine);
        if (!trimmedNext.empty() && trimmedNext[0] != '#')
        {
          foundNext = true;
          break;
        }
      }
      
      if (foundNext)
      {
        int nextIndent = GetIndent(nextLine);
        stream.seekg(pos);
        
        if (nextIndent > indent)
        {
          // Nested object or array
          if (Trim(nextLine)[0] == '-')
          {
            auto arrayResult = ParseArray(stream, line, nextIndent);
            if (arrayResult.HasError())
            {
              return arrayResult;
            }
            result.SetObjectField(key, arrayResult.GetValue());
          }
          else
          {
            auto objResult = ParseObject(stream, line, nextIndent);
            if (objResult.HasError())
            {
              return objResult;
            }
            result.SetObjectField(key, objResult.GetValue());
          }
        }
        else
        {
          result.SetObjectField(key, YamlValue());
        }
      }
      else
      {
        result.SetObjectField(key, YamlValue());
      }
    }
    else if (value[0] == '[')
    {
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
        
        std::string elem = Trim(value.substr(start, end - start));
        if (!elem.empty())
        {
          auto elemResult = ParseScalar(elem);
          if (elemResult.HasValue())
          {
            arr.AddArrayElement(elemResult.GetValue());
          }
        }
        start = end + 1;
      }
      result.SetObjectField(key, arr);
    }
    else
    {
      // Simple value
      auto valResult = ParseScalar(value);
      if (valResult.HasError())
      {
        return valResult;
      }
      result.SetObjectField(key, valResult.GetValue());
    }
  }
  
  return Result<YamlValue>(result);
}

std::string YamlParser::Trim(const std::string& str)
{
  size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

int YamlParser::GetIndent(const std::string& line)
{
  int indent = 0;
  for (char c : line)
  {
    if (c == ' ') indent++;
    else if (c == '\t') indent += 2;
    else break;
  }
  return indent;
}

std::string YamlParser::Unquote(const std::string& str)
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
Result<LaunchDescriptionPtr> YamlLaunchBuilder::Build(const YamlValue& yaml)
{
  if (!yaml.IsObject())
  {
    return Result<LaunchDescriptionPtr>(Error(ErrorCode::kInvalidConfiguration, "Root must be an object"));
  }
  
  auto desc = std::make_shared<LaunchDescription>();
  
  auto entities = yaml.AsObject().find("entities");
  if (entities == yaml.AsObject().end() || !entities->second.IsArray())
  {
    return Result<LaunchDescriptionPtr>(Error(ErrorCode::kInvalidConfiguration, "Missing 'entities' array"));
  }
  
  for (const auto& entityYaml : entities->second.AsArray())
  {
    if (!entityYaml.IsObject())
    {
      continue;
    }
    
    auto actionResult = BuildAction(entityYaml);
    if (actionResult.HasError())
    {
      continue;
    }
    
    desc->Add(actionResult.GetValue());
  }
  
  return Result<LaunchDescriptionPtr>(desc);
}

Result<ActionPtr> YamlLaunchBuilder::BuildAction(const YamlValue& actionYaml)
{
  auto typeIt = actionYaml.AsObject().find("type");
  if (typeIt == actionYaml.AsObject().end() || !typeIt->second.IsString())
  {
    return Result<ActionPtr>(Error(ErrorCode::kInvalidConfiguration, "Action missing 'type' field"));
  }
  
  std::string type = typeIt->second.AsString();
  
  if (type == "execute_process")
  {
    ExecuteProcess::Options options;
    
    // Parse process name
    auto nameIt = actionYaml.AsObject().find("name");
    if (nameIt != actionYaml.AsObject().end() && nameIt->second.IsString())
    {
      options.name = std::make_shared<TextSubstitution>(nameIt->second.AsString());
    }
    
    // Parse command with variable substitution support
    auto cmdIt = actionYaml.AsObject().find("cmd");
    if (cmdIt != actionYaml.AsObject().end() && cmdIt->second.IsArray())
    {
      for (const auto& cmdElem : cmdIt->second.AsArray())
      {
        if (cmdElem.IsString())
        {
          auto substResult = BuildSubstitution(cmdElem.AsString());
          if (substResult.HasValue())
          {
            options.cmd.push_back(substResult.GetValue());
          }
          else
          {
            // Fallback to text if substitution building fails
            options.cmd.push_back(std::make_shared<TextSubstitution>(cmdElem.AsString()));
          }
        }
      }
    }
    
    // Parse output
    auto outputIt = actionYaml.AsObject().find("output");
    if (outputIt != actionYaml.AsObject().end() && outputIt->second.IsString())
    {
      options.output = outputIt->second.AsString();
    }
    
    // Parse dependencies
    auto dependsIt = actionYaml.AsObject().find("depends_on");
    if (dependsIt != actionYaml.AsObject().end() && dependsIt->second.IsArray())
    {
      for (const auto& depElem : dependsIt->second.AsArray())
      {
        if (depElem.IsString())
        {
          options.dependsOn.push_back(depElem.AsString());
        }
      }
    }
    
    return Result<ActionPtr>(std::make_shared<ExecuteProcess>(options));
  }
  else if (type == "declare_launch_argument")
  {
    DeclareLaunchArgument::Options options;
    
    auto nameIt = actionYaml.AsObject().find("name");
    if (nameIt != actionYaml.AsObject().end() && nameIt->second.IsString())
    {
      options.name = nameIt->second.AsString();
    }
    
    auto defaultIt = actionYaml.AsObject().find("default_value");
    if (defaultIt != actionYaml.AsObject().end() && defaultIt->second.IsString())
    {
      options.defaultValue = std::make_shared<TextSubstitution>(defaultIt->second.AsString());
    }
    
    auto descIt = actionYaml.AsObject().find("description");
    if (descIt != actionYaml.AsObject().end() && descIt->second.IsString())
    {
      options.description = descIt->second.AsString();
    }
    
    return Result<ActionPtr>(std::make_shared<DeclareLaunchArgument>(options));
  }
  
  return Result<ActionPtr>(Error(ErrorCode::kNotImplemented, "Unknown action type: " + type));
}

Result<SubstitutionPtr> YamlLaunchBuilder::BuildSubstitution(const std::string& value)
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
        // TODO: Implement find_package substitution
        return Result<SubstitutionPtr>(std::make_shared<TextSubstitution>(value));
      }
      else if (subst_type == "env" && space_pos != std::string::npos)
      {
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

Result<ConditionPtr> YamlLaunchBuilder::BuildCondition(const YamlValue& conditionYaml)
{
  (void)conditionYaml;
  // TODO: Implement condition building
  return Result<ConditionPtr>(Error(ErrorCode::kNotImplemented, "Condition building not yet implemented"));
}

}  // namespace launch_cpp
