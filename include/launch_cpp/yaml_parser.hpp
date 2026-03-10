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


#ifndef LAUNCH_CPP__YAML_PARSER_HPP_
#define LAUNCH_CPP__YAML_PARSER_HPP_

// Simple YAML parser for launch files
// Note: This is a minimal implementation for demo purposes
// In production, use yaml-cpp library

#include "launch_cpp/error_code.hpp"
#include "launch_cpp/launch_description.hpp"
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>

namespace launch_cpp
{

// Forward declarations
class YamlValue;
class YamlNode;

// YAML value types
enum class YamlType
{
  kNull,
  kString,
  kNumber,
  kBoolean,
  kArray,
  kObject
};

// Simple YAML value wrapper
class YamlValue
{
 public:
  YamlValue() : type_(YamlType::kNull) {}
  explicit YamlValue(const std::string& value) : type_(YamlType::kString), stringValue_(value) {}
  explicit YamlValue(double value) : type_(YamlType::kNumber), numberValue_(value) {}
  explicit YamlValue(bool value) : type_(YamlType::kBoolean), boolValue_(value) {}
  
  YamlType get_type() const { return type_; }
  
  bool is_null() const { return type_ == YamlType::kNull; }
  bool is_string() const { return type_ == YamlType::kString; }
  bool IsNumber() const { return type_ == YamlType::kNumber; }
  bool IsBoolean() const { return type_ == YamlType::kBoolean; }
  bool IsArray() const { return type_ == YamlType::kArray; }
  bool IsObject() const { return type_ == YamlType::kObject; }
  
  const std::string& as_string() const { return stringValue_; }
  double AsNumber() const { return numberValue_; }
  bool AsBoolean() const { return boolValue_; }
  
  const std::vector<YamlValue>& AsArray() const { return arrayValue_; }
  std::vector<YamlValue>& AsArray() { return arrayValue_; }
  
  const std::map<std::string, YamlValue>& AsObject() const { return objectValue_; }
  std::map<std::string, YamlValue>& AsObject() { return objectValue_; }
  
  void AddArrayElement(const YamlValue& value) { type_ = YamlType::kArray; arrayValue_.push_back(value); }
  void SetObjectField(const std::string& key, const YamlValue& value) { type_ = YamlType::kObject; objectValue_[key] = value; }
  
 private:
  YamlType type_;
  std::string stringValue_;
  double numberValue_ = 0.0;
  bool boolValue_ = false;
  std::vector<YamlValue> arrayValue_;
  std::map<std::string, YamlValue> objectValue_;
};

// YAML parser
class YamlParser
{
 public:
  static Result<YamlValue> Parse(const std::string& content);
  static Result<YamlValue> ParseFile(const std::string& filePath);
  
 private:
  static Result<YamlValue> ParseValue(std::istringstream& stream, int& line);
  static Result<YamlValue> ParseScalar(const std::string& value);
  static Result<YamlValue> ParseArray(std::istringstream& stream, int& line, int indent);
  static Result<YamlValue> ParseObject(std::istringstream& stream, int& line, int indent);
  
  // Helper for parsing object elements in arrays
  static YamlValue ParseArrayElementObject(std::istringstream& stream, int& line, 
                                           const std::string& firstLine, int baseIndent);
  
  static std::string Trim(const std::string& str);
  static int GetIndent(const std::string& line);
  static std::string Unquote(const std::string& str);
};

// Launch description builder from YAML
class YamlLaunchBuilder
{
 public:
  static Result<LaunchDescriptionPtr> Build(const YamlValue& yaml);
  
 private:
  static Result<ActionPtr> BuildAction(const YamlValue& actionYaml);
  static Result<SubstitutionPtr> BuildSubstitution(const std::string& value);
  static Result<ConditionPtr> BuildCondition(const YamlValue& conditionYaml);
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__YAML_PARSER_HPP_
