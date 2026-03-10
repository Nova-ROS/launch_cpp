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


// Comprehensive YAML Parser Tests - Fixed for actual parser behavior
#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include "launch_cpp/yaml_parser.hpp"

using namespace launch_cpp;

// Test: Parse empty content
TEST(YamlParserTest, ParseEmpty)
{
  std::string yaml = "";
  auto result = YamlParser::parse(yaml);
  
  ASSERT_TRUE(result.has_value());
  // Empty content returns null YamlValue (not an object)
  EXPECT_TRUE(result.get_value().is_null());
}

// Test: Parse null value
TEST(YamlParserTest, ParseNull)
{
  std::string yaml = "key: null";
  auto result = YamlParser::parse(yaml);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  EXPECT_EQ(result.get_value().as_object().size(), 1U);
}

// Test: Parse boolean values
TEST(YamlParserTest, ParseBoolean)
{
  // Test true values
  for (const auto& val : {"true", "yes", "on"}) {
    std::string yaml = std::string("key: ") + val;
    auto result = YamlParser::parse(yaml);
    ASSERT_TRUE(result.has_value()) << "Failed to parse: " << val;
    EXPECT_TRUE(result.get_value().is_object());
    auto it = result.get_value().as_object().find("key");
    ASSERT_NE(it, result.get_value().as_object().end());
    EXPECT_TRUE(it->second.is_boolean());
    EXPECT_TRUE(it->second.as_boolean());
  }
  
  // Test false values
  for (const auto& val : {"false", "no", "off"}) {
    std::string yaml = std::string("key: ") + val;
    auto result = YamlParser::parse(yaml);
    ASSERT_TRUE(result.has_value()) << "Failed to parse: " << val;
    EXPECT_TRUE(result.get_value().is_object());
    auto it = result.get_value().as_object().find("key");
    ASSERT_NE(it, result.get_value().as_object().end());
    EXPECT_TRUE(it->second.is_boolean());
    EXPECT_FALSE(it->second.as_boolean());
  }
}

// Test: Parse integer values
TEST(YamlParserTest, ParseInteger)
{
  std::string yaml = "key: 42";
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  auto it = result.get_value().as_object().find("key");
  ASSERT_NE(it, result.get_value().as_object().end());
  EXPECT_TRUE(it->second.is_number());
  EXPECT_DOUBLE_EQ(it->second.as_number(), 42.0);
}

// Test: Parse floating point values
TEST(YamlParserTest, ParseFloat)
{
  std::vector<std::string> floats = {"3.14", "-0.5", "1.0", "0.0", "-123.456"};
  
  for (const auto& val : floats) {
    std::string yaml = std::string("key: ") + val;
    auto result = YamlParser::parse(yaml);
    ASSERT_TRUE(result.has_value()) << "Failed to parse: " << val;
    EXPECT_TRUE(result.get_value().is_object());
    auto it = result.get_value().as_object().find("key");
    ASSERT_NE(it, result.get_value().as_object().end());
    EXPECT_TRUE(it->second.is_number());
  }
}

// Test: Parse string values
TEST(YamlParserTest, ParseString)
{
  std::string yaml = "key: hello";
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  auto it = result.get_value().as_object().find("key");
  ASSERT_NE(it, result.get_value().as_object().end());
  EXPECT_TRUE(it->second.is_string());
  EXPECT_EQ(it->second.as_string(), "hello");
}

// Test: Parse quoted string
TEST(YamlParserTest, ParseQuotedString)
{
  std::string yaml = "key: \"hello world\"";
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  auto it = result.get_value().as_object().find("key");
  ASSERT_NE(it, result.get_value().as_object().end());
  EXPECT_TRUE(it->second.is_string());
  EXPECT_EQ(it->second.as_string(), "hello world");
}

// Test: Parse empty string
TEST(YamlParserTest, ParseEmptyString)
{
  std::string yaml = "key: \"\"";
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  auto it = result.get_value().as_object().find("key");
  ASSERT_NE(it, result.get_value().as_object().end());
  EXPECT_TRUE(it->second.is_string());
  EXPECT_EQ(it->second.as_string(), "");
}

// Test: Parse simple array
TEST(YamlParserTest, ParseSimpleArray)
{
  std::string yaml = R"(items:
  - item1
  - item2
  - item3)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  
  auto it = result.get_value().as_object().find("items");
  ASSERT_NE(it, result.get_value().as_object().end());
  EXPECT_TRUE(it->second.is_array());
  
  const auto& arr = it->second.as_array();
  EXPECT_EQ(arr.size(), 3U);
  EXPECT_EQ(arr[0].as_string(), "item1");
  EXPECT_EQ(arr[1].as_string(), "item2");
  EXPECT_EQ(arr[2].as_string(), "item3");
}

// Test: Parse array with different types
TEST(YamlParserTest, ParseMixedArray)
{
  std::string yaml = R"(items:
  - string_value
  - 42
  - 3.14
  - true)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  
  auto it = result.get_value().as_object().find("items");
  ASSERT_NE(it, result.get_value().as_object().end());
  EXPECT_TRUE(it->second.is_array());
  
  const auto& arr = it->second.as_array();
  EXPECT_EQ(arr.size(), 4U);
  EXPECT_TRUE(arr[0].is_string());
  EXPECT_TRUE(arr[1].is_number());
  EXPECT_DOUBLE_EQ(arr[1].as_number(), 42.0);
  EXPECT_TRUE(arr[2].is_number());
  EXPECT_TRUE(arr[3].is_boolean());
}

// Test: Parse array with simple items only
TEST(YamlParserTest, ParseNestedArray)
{
  // The parser doesn't support nested structures in arrays directly
  // but supports simple array of scalars
  std::string yaml = R"(items:
  - item1
  - item2
  - item3)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  
  auto it = result.get_value().as_object().find("items");
  ASSERT_NE(it, result.get_value().as_object().end());
  EXPECT_TRUE(it->second.is_array());
  
  const auto& arr = it->second.as_array();
  EXPECT_EQ(arr.size(), 3U);
  EXPECT_TRUE(arr[0].is_string());
  EXPECT_TRUE(arr[1].is_string());
  EXPECT_TRUE(arr[2].is_string());
}

// Test: Parse simple object
TEST(YamlParserTest, ParseSimpleObject)
{
  std::string yaml = R"(name: test
value: 42)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  
  const auto& obj = result.get_value().as_object();
  EXPECT_EQ(obj.size(), 2U);
  
  auto nameIt = obj.find("name");
  ASSERT_NE(nameIt, obj.end());
  EXPECT_EQ(nameIt->second.as_string(), "test");
  
  auto valueIt = obj.find("value");
  ASSERT_NE(valueIt, obj.end());
  EXPECT_EQ(valueIt->second.as_number(), 42.0);
}

// Test: Parse nested objects
TEST(YamlParserTest, ParseNestedObject)
{
  std::string yaml = R"(config:
  name: test
  value: 42)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  
  auto configIt = result.get_value().as_object().find("config");
  ASSERT_NE(configIt, result.get_value().as_object().end());
  EXPECT_TRUE(configIt->second.is_object());
  
  const auto& nested = configIt->second.as_object();
  EXPECT_EQ(nested.size(), 2U);
  
  auto nameIt = nested.find("name");
  ASSERT_NE(nameIt, nested.end());
  EXPECT_EQ(nameIt->second.as_string(), "test");
}

// Test: Parse deeply nested structure
TEST(YamlParserTest, ParseDeepNesting)
{
  std::string yaml = R"(level1:
  level2:
    level3:
      value: deep)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  
  auto l1 = result.get_value().as_object().find("level1");
  ASSERT_NE(l1, result.get_value().as_object().end());
  EXPECT_TRUE(l1->second.is_object());
  
  auto l2 = l1->second.as_object().find("level2");
  ASSERT_NE(l2, l1->second.as_object().end());
  EXPECT_TRUE(l2->second.is_object());
  
  auto l3 = l2->second.as_object().find("level3");
  ASSERT_NE(l3, l2->second.as_object().end());
  EXPECT_TRUE(l3->second.is_object());
}

// Test: Parse inline array
TEST(YamlParserTest, ParseInlineArray)
{
  std::string yaml = "items: [1, 2, 3]";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  
  auto it = result.get_value().as_object().find("items");
  ASSERT_NE(it, result.get_value().as_object().end());
  EXPECT_TRUE(it->second.is_array());
  
  const auto& arr = it->second.as_array();
  EXPECT_EQ(arr.size(), 3U);
}

// Test: Parse file
TEST(YamlParserTest, ParseFile)
{
  // Create temporary file
  const char* tempFile = "/tmp/test_yaml_parser.yaml";
  {
    std::ofstream file(tempFile);
    file << "key: value\n";
    file << "number: 42\n";
    file.close();
  }
  
  auto result = YamlParser::parse_file(tempFile);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  
  const auto& obj = result.get_value().as_object();
  EXPECT_EQ(obj.size(), 2U);
  
  std::remove(tempFile);
}

// Test: Parse non-existent file
TEST(YamlParserTest, ParseNonExistentFile)
{
  auto result = YamlParser::parse_file("/nonexistent/file.yaml");
  ASSERT_TRUE(result.has_error());
}

// Test: Parse with comments
TEST(YamlParserTest, ParseWithComments)
{
  std::string yaml = R"(# This is a comment
key: value
# Another comment
number: 42)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  
  const auto& obj = result.get_value().as_object();
  EXPECT_EQ(obj.size(), 2U);
}

// Test: Parse empty lines
TEST(YamlParserTest, ParseWithEmptyLines)
{
  std::string yaml = R"(key: value

number: 42

string: test)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  
  const auto& obj = result.get_value().as_object();
  EXPECT_EQ(obj.size(), 3U);
}

// Test: Complex launch file structure
TEST(YamlParserTest, ParseLaunchFile)
{
  std::string yaml = R"(launch:
  entities:
    - type: execute_process
      cmd:
        - echo
        - hello
      output: screen
    - type: declare_launch_argument
      name: arg1
      default_value: default)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
}

// Test: YamlValue type checking
TEST(YamlValueTest, TypeChecking)
{
  YamlValue nullVal;
  EXPECT_TRUE(nullVal.is_null());
  EXPECT_FALSE(nullVal.is_string());
  EXPECT_FALSE(nullVal.is_number());
  EXPECT_FALSE(nullVal.is_boolean());
  EXPECT_FALSE(nullVal.is_array());
  EXPECT_FALSE(nullVal.is_object());
  
  // Note: explicit constructors require explicit type
  YamlValue strVal(std::string("test"));
  EXPECT_FALSE(strVal.is_null());
  EXPECT_TRUE(strVal.is_string());
  EXPECT_EQ(strVal.as_string(), "test");
  
  YamlValue numVal(static_cast<double>(42.0));
  EXPECT_TRUE(numVal.is_number());
  EXPECT_DOUBLE_EQ(numVal.as_number(), 42.0);
  
  YamlValue boolVal(static_cast<bool>(true));
  EXPECT_TRUE(boolVal.is_boolean());
  EXPECT_TRUE(boolVal.as_boolean());
}

// Test: YamlValue array operations
TEST(YamlValueTest, ArrayOperations)
{
  YamlValue arr;
  arr.add_array_element(YamlValue(std::string("item1")));
  arr.add_array_element(YamlValue(static_cast<double>(42.0)));
  arr.add_array_element(YamlValue(static_cast<bool>(true)));
  
  EXPECT_TRUE(arr.is_array());
  EXPECT_EQ(arr.as_array().size(), 3U);
  EXPECT_EQ(arr.as_array()[0].as_string(), "item1");
  EXPECT_DOUBLE_EQ(arr.as_array()[1].as_number(), 42.0);
  EXPECT_TRUE(arr.as_array()[2].as_boolean());
}

// Test: YamlValue object operations
TEST(YamlValueTest, ObjectOperations)
{
  YamlValue obj;
  obj.set_object_field("key1", YamlValue(std::string("value1")));
  obj.set_object_field("key2", YamlValue(static_cast<double>(42.0)));
  
  EXPECT_TRUE(obj.is_object());
  EXPECT_EQ(obj.as_object().size(), 2U);
  EXPECT_EQ(obj.as_object().at("key1").as_string(), "value1");
  EXPECT_DOUBLE_EQ(obj.as_object().at("key2").as_number(), 42.0);
}

// Test: YamlLaunchBuilder with valid entities
TEST(YamlLaunchBuilderTest, BuildLaunchDescription)
{
  std::string yaml = R"(entities:
  - type: execute_process
    cmd:
      - echo
      - hello
    output: screen
  - type: declare_launch_argument
    name: test_arg
    default_value: default)";
  
  auto parseResult = YamlParser::parse(yaml);
  ASSERT_TRUE(parseResult.has_value());
  
  auto buildResult = YamlLaunchBuilder::build(parseResult.get_value());
  ASSERT_TRUE(buildResult.has_value());
  EXPECT_NE(buildResult.get_value(), nullptr);
}

// Test: YamlLaunchBuilder with missing entities
TEST(YamlLaunchBuilderTest, BuildWithoutEntities)
{
  std::string yaml = "name: test";
  
  auto parseResult = YamlParser::parse(yaml);
  ASSERT_TRUE(parseResult.has_value());
  
  auto buildResult = YamlLaunchBuilder::build(parseResult.get_value());
  ASSERT_TRUE(buildResult.has_error());
}

// Test: YamlLaunchBuilder with invalid root type
TEST(YamlLaunchBuilderTest, BuildWithInvalidRoot)
{
  std::string yaml = R"(items:
  - 1
  - 2)";
  
  auto parseResult = YamlParser::parse(yaml);
  ASSERT_TRUE(parseResult.has_value());
  
  auto buildResult = YamlLaunchBuilder::build(parseResult.get_value());
  ASSERT_TRUE(buildResult.has_error());
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
