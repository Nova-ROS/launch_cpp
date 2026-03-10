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

void LaunchDescription::add(const LaunchDescriptionEntityPtr& entity)
{
  entities_.push_back(entity);
}

void LaunchDescription::add(LaunchDescriptionEntityPtr&& entity)
{
  entities_.push_back(std::move(entity));
}

Result<LaunchDescriptionEntityVector> LaunchDescription::visit(LaunchContext& context)
{
  LaunchDescriptionEntityVector result;
  result.reserve(entities_.size());

  for (LaunchDescriptionEntityPtr& entity : entities_)
  {
    if (!entity)
    {
      continue;
    }

    Result<LaunchDescriptionEntityVector> visitResult = entity->visit(context);

    if (visitResult.has_error())
    {
      return visitResult;
    }

    LaunchDescriptionEntityVector& childEntities = visitResult.get_value();

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

Result<LaunchDescriptionPtr> LaunchDescription::from_yaml(const std::string& yamlString)
{
  auto yamlResult = YamlParser::parse(yamlString);
  if (yamlResult.has_error())
  {
    return Result<LaunchDescriptionPtr>(yamlResult.get_error());
  }

  return YamlLaunchBuilder::build(yamlResult.get_value());
}

Result<LaunchDescriptionPtr> LaunchDescription::from_yaml_file(const std::string& filePath)
{
  auto yamlResult = YamlParser::parse_file(filePath);
  if (yamlResult.has_error())
  {
    return Result<LaunchDescriptionPtr>(yamlResult.get_error());
  }

  return YamlLaunchBuilder::build(yamlResult.get_value());
}

}  // namespace launch_cpp
