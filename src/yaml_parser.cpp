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


#include "cpp_launch/yaml_parser.hpp"
#include "cpp_launch/actions/execute_process.hpp"
#include "cpp_launch/actions/declare_launch_argument.hpp"
#include "cpp_launch/substitutions/text_substitution.hpp"
#include "cpp_launch/substitutions/launch_configuration.hpp"
#include "cpp_launch/conditions/if_condition.hpp"
#include <cctype>
#include <algorithm>

namespace cpp_launch
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
    
    if (Trim(lineStr).empty() || Trim(lineStr)[0] == '#')
    {
      continue;
    }
    
    int indent = GetIndent(lineStr);
    if (indent < baseIndent)
    {
      // End of array
      break;
    }
    
    std::string trimmed = Trim(lineStr);
    if (trimmed[0] == '-')
    {
      trimmed = Trim(trimmed.substr(1));
      
      if (trimmed.empty())
      {
        // Array of objects or nested arrays
        auto nextResult = ParseValue(stream, line);
        if (nextResult.HasError())
        {
          return nextResult;
        }
        result.AddArrayElement(nextResult.GetValue());
      }
      else
      {
        // Simple array element
        auto scalarResult = ParseScalar(trimmed);
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
      // Log warning but continue
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
    
    auto cmdIt = actionYaml.AsObject().find("cmd");
    if (cmdIt != actionYaml.AsObject().end() && cmdIt->second.IsArray())
    {
      for (const auto& cmdElem : cmdIt->second.AsArray())
      {
        if (cmdElem.IsString())
        {
          options.cmd.push_back(std::make_shared<TextSubstitution>(cmdElem.AsString()));
        }
      }
    }
    
    auto outputIt = actionYaml.AsObject().find("output");
    if (outputIt != actionYaml.AsObject().end() && outputIt->second.IsString())
    {
      options.output = outputIt->second.AsString();
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
  // Simple implementation - just treat as text
  return Result<SubstitutionPtr>(std::make_shared<TextSubstitution>(value));
}

Result<ConditionPtr> YamlLaunchBuilder::BuildCondition(const YamlValue& conditionYaml)
{
  (void)conditionYaml;
  // TODO: Implement condition building
  return Result<ConditionPtr>(Error(ErrorCode::kNotImplemented, "Condition building not yet implemented"));
}

}  // namespace cpp_launch
