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


#include "launch_cpp/launch_description.hpp"
#include "launch_cpp/launch_context.hpp"
#include "launch_cpp/launch_description_entity.hpp"
#include "launch_cpp/error_code.hpp"
#include "launch_cpp/yaml_parser.hpp"

namespace launch_cpp
{

LaunchDescription::LaunchDescription(const LaunchDescriptionEntityVector& entities)
  : entities_(entities)
{
}

LaunchDescription::LaunchDescription(LaunchDescriptionEntityVector&& entities)
  : entities_(std::move(entities))
{
}

void LaunchDescription::Add(const LaunchDescriptionEntityPtr& entity)
{
  entities_.push_back(entity);
}

void LaunchDescription::Add(LaunchDescriptionEntityPtr&& entity)
{
  entities_.push_back(std::move(entity));
}

Result<LaunchDescriptionEntityVector> LaunchDescription::Visit(LaunchContext& context)
{
  LaunchDescriptionEntityVector result;
  result.reserve(entities_.size());
  
  for (LaunchDescriptionEntityPtr& entity : entities_)
  {
    if (!entity)
    {
      continue;
    }
    
    Result<LaunchDescriptionEntityVector> visitResult = entity->Visit(context);
    
    if (visitResult.HasError())
    {
      return visitResult;
    }
    
    LaunchDescriptionEntityVector& childEntities = visitResult.GetValue();
    
    for (LaunchDescriptionEntityPtr& child : childEntities)
    {
      if (child)
      {
        result.push_back(std::move(child));
      }
    }
  }
  
  return Result<LaunchDescriptionEntityVector>(std::move(result));
}

Result<LaunchDescriptionPtr> LaunchDescription::FromYaml(const std::string& yamlString)
{
  auto yamlResult = YamlParser::Parse(yamlString);
  if (yamlResult.HasError())
  {
    return Result<LaunchDescriptionPtr>(yamlResult.GetError());
  }
  
  return YamlLaunchBuilder::Build(yamlResult.GetValue());
}

Result<LaunchDescriptionPtr> LaunchDescription::FromYamlFile(const std::string& filePath)
{
  auto yamlResult = YamlParser::ParseFile(filePath);
  if (yamlResult.HasError())
  {
    return Result<LaunchDescriptionPtr>(yamlResult.GetError());
  }
  
  return YamlLaunchBuilder::Build(yamlResult.GetValue());
}

}  // namespace launch_cpp
