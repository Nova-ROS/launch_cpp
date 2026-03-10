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
  explicit YamlValue(const std::string& value) : type_(YamlType::kString), string_value_(value) {}
  explicit YamlValue(double value) : type_(YamlType::kNumber), number_value_(value) {}
  explicit YamlValue(bool value) : type_(YamlType::kBoolean), bool_value_(value) {}
  
  YamlType get_type() const { return type_; }
  
  bool is_null() const { return type_ == YamlType::kNull; }
  bool is_string() const { return type_ == YamlType::kString; }
  bool is_number() const { return type_ == YamlType::kNumber; }
  bool is_boolean() const { return type_ == YamlType::kBoolean; }
  bool is_array() const { return type_ == YamlType::kArray; }
  bool is_object() const { return type_ == YamlType::kObject; }
  
  const std::string& as_string() const { return string_value_; }
  double as_number() const { return number_value_; }
  bool as_boolean() const { return bool_value_; }
  
  const std::vector<YamlValue>& as_array() const { return array_value_; }
  std::vector<YamlValue>& as_array() { return array_value_; }
  
  const std::map<std::string, YamlValue>& as_object() const { return object_value_; }
  std::map<std::string, YamlValue>& as_object() { return object_value_; }
  
  void add_array_element(const YamlValue& value) { type_ = YamlType::kArray; array_value_.push_back(value); }
  void set_object_field(const std::string& key, const YamlValue& value) { type_ = YamlType::kObject; object_value_[key] = value; }
  
 private:
  YamlType type_;
  std::string string_value_;
  double number_value_ = 0.0;
  bool bool_value_ = false;
  std::vector<YamlValue> array_value_;
  std::map<std::string, YamlValue> object_value_;
};

// YAML parser
class YamlParser
{
 public:
  static Result<YamlValue> parse(const std::string& content);
  static Result<YamlValue> parse_file(const std::string& filePath);
  
 private:
  static Result<YamlValue> parse_value(std::istringstream& stream, int& line);
  static Result<YamlValue> parse_scalar(const std::string& value);
  static Result<YamlValue> parse_array(std::istringstream& stream, int& line, int indent);
  static Result<YamlValue> parse_object(std::istringstream& stream, int& line, int indent);
  
  // Helper for parsing object elements in arrays
  static YamlValue parse_array_element_object(std::istringstream& stream, int& line, 
                                           const std::string& firstLine, int baseIndent);
  
  static std::string trim(const std::string& str);
  static int get_indent(const std::string& line);
  static std::string unquote(const std::string& str);
};

// Launch description builder from YAML
class YamlLaunchBuilder
{
 public:
  static Result<LaunchDescriptionPtr> build(const YamlValue& yaml);
  
 private:
  static Result<ActionPtr> build_action(const YamlValue& actionYaml);
  static Result<SubstitutionPtr> build_substitution(const std::string& value);
  static Result<ConditionPtr> build_condition(const YamlValue& conditionYaml);
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__YAML_PARSER_HPP_
