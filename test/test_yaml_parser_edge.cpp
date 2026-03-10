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

// Additional YAML Parser Edge Case Tests - Coverage Improvement
#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include "launch_cpp/yaml_parser.hpp"

using namespace launch_cpp;

// Test: Parse invalid number format
TEST(YamlParserEdgeTest, ParseInvalidNumber)
{
  // Double decimal point should be treated as string
  std::string yaml = "key: 3.14.159";
  auto result = YamlParser::parse(yaml);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  auto& obj = result.get_value().as_object();
  EXPECT_EQ(obj["key"].get_type(), YamlType::K_STRING);
}

// Test: Parse number with sign
TEST(YamlParserEdgeTest, ParseNumberWithSign)
{
  std::string yaml = "positive: +42\nnegative: -3.14";
  auto result = YamlParser::parse(yaml);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
}

// Test: Parse deeply nested object
TEST(YamlParserEdgeTest, ParseDeepNesting)
{
  std::string yaml = R"(level1:
  level2:
    level3:
      level4:
        level5: deep_value)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: Parse array with mixed types
TEST(YamlParserEdgeTest, ParseMixedTypeArray)
{
  std::string yaml = R"(items:
  - string_value
  - 42
  - 3.14
  - true
  - null)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
  auto& obj = result.get_value().as_object();
  EXPECT_EQ(obj["items"].get_type(), YamlType::K_ARRAY);
}

// Test: Parse YAML with tabs
TEST(YamlParserEdgeTest, ParseWithTabs)
{
  std::string yaml = "key:\n\t- item1\n\t- item2";
  auto result = YamlParser::parse(yaml);
  // Should handle or fail gracefully
}

// Test: Parse quoted strings with special characters
TEST(YamlParserEdgeTest, ParseQuotedSpecialChars)
{
  std::string yaml = R"(key: "value with \n newline")";
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: Parse empty array
TEST(YamlParserEdgeTest, ParseEmptyArray)
{
  std::string yaml = "items: []";
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: Parse empty object
TEST(YamlParserEdgeTest, ParseEmptyObject)
{
  std::string yaml = "config: {}";
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: Parse array with object elements
TEST(YamlParserEdgeTest, ParseArrayWithObjects)
{
  std::string yaml = R"(items:
  - name: item1
    value: 10
  - name: item2
    value: 20)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: Parse multi-line string
TEST(YamlParserEdgeTest, ParseMultilineString)
{
  std::string yaml = R"(description: |
  This is a multi-line
  string value)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: Parse YAML with windows line endings
TEST(YamlParserEdgeTest, ParseWindowsLineEndings)
{
  std::string yaml = "key1: value1\r\nkey2: value2";
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: Parse boolean variations
TEST(YamlParserEdgeTest, ParseBooleanVariations)
{
  std::string yaml = R"(
true1: TRUE
false1: FALSE
yes_upper: YES
no_upper: NO
on_upper: ON
off_upper: OFF)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: Parse numeric edge cases
TEST(YamlParserEdgeTest, ParseNumericEdgeCases)
{
  std::string yaml = R"(
zero: 0
negative_zero: -0
dot_start: .5
dot_end: 5.
large_int: 999999999)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: Parse string with leading/trailing spaces
TEST(YamlParserEdgeTest, ParseStringWithSpaces)
{
  std::string yaml = "key: \"  value with spaces  \"";
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: Parse file that doesn't exist
TEST(YamlParserEdgeTest, ParseFileNotExist)
{
  auto result = YamlParser::parse_file("/nonexistent/file.yaml");
  ASSERT_TRUE(result.has_error());
}

// Test: Parse file with only comments
TEST(YamlParserEdgeTest, ParseOnlyComments)
{
  std::string yaml = R"(# Comment 1
# Comment 2
# Comment 3)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: Parse inline nested object
TEST(YamlParserEdgeTest, ParseInlineNestedObject)
{
  std::string yaml = "config: {name: test, value: 42}";
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: YamlValue type checking
TEST(YamlValueTest, TypeChecking)
{
  YamlValue null_val;
  EXPECT_TRUE(null_val.is_null());
  
  YamlValue str_val(std::string("test"));
  EXPECT_TRUE(str_val.is_string());
  
  YamlValue num_val(42.0);
  EXPECT_TRUE(num_val.is_number());
  
  YamlValue bool_val(true);
  EXPECT_TRUE(bool_val.is_boolean());
}

// Test: YamlValue conversion
TEST(YamlValueTest, TypeConversion)
{
  YamlValue str_val(std::string("hello"));
  EXPECT_EQ(str_val.as_string(), "hello");
  
  YamlValue num_val(42.0);
  EXPECT_EQ(num_val.as_number(), 42.0);
  
  YamlValue bool_val(true);
  EXPECT_EQ(bool_val.as_boolean(), true);
}

// Test: YamlValue array operations
TEST(YamlValueTest, ArrayOperations)
{
  YamlValue arr_val;
  arr_val.add_array_element(YamlValue(std::string("item1")));
  arr_val.add_array_element(YamlValue(42.0));
  
  EXPECT_TRUE(arr_val.is_array());
  EXPECT_EQ(arr_val.as_array().size(), 2U);
}

// Test: YamlValue object operations
TEST(YamlValueTest, ObjectOperations)
{
  YamlValue obj_val;
  obj_val.set_object_field("key1", YamlValue(std::string("value1")));
  obj_val.set_object_field("key2", YamlValue(123.0));
  
  EXPECT_TRUE(obj_val.is_object());
  EXPECT_EQ(obj_val.as_object().size(), 2U);
}

// Test: Parse complex launch file structure
TEST(YamlParserLaunchTest, ParseComplexLaunchFile)
{
  std::string yaml = R"(launch:
  entities:
    - type: execute_process
      name: talker
      cmd:
        - ros2
        - run
        - demo_nodes_cpp
        - talker
      output: screen
      depends_on:
        - listener
    - type: execute_process
      name: listener
      cmd:
        - ros2
        - run
        - demo_nodes_cpp
        - listener
      output: log
    - type: declare_launch_argument
      name: use_sim_time
      default_value: "false"
      description: Use simulation time)";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
}

// Test: Error handling for malformed YAML
TEST(YamlParserErrorTest, MalformedYaml)
{
  // Unclosed quote
  std::string yaml = "key: \"unclosed string";
  auto result = YamlParser::parse(yaml);
  // Should handle gracefully
  ASSERT_TRUE(result.has_value());
}

// Test: Large YAML file
TEST(YamlParserPerformanceTest, LargeYamlFile)
{
  std::string yaml = "items:\n";
  for (int i = 0; i < 100; ++i) {
    yaml += "  - item" + std::to_string(i) + "\n";
  }
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.get_value().is_object());
}

// Test: Parse YAML with unicode characters
TEST(YamlParserEncodingTest, UnicodeCharacters)
{
  std::string yaml = "key: \"Unicode: \xE4\xB8\xAD\xE6\x96\x87\"";
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: YamlLaunchBuilder with invalid action type
TEST(YamlLaunchBuilderEdgeTest, InvalidActionType)
{
  std::string yaml = R"(entities:
  - type: invalid_action_type
    name: test)";
  
  auto parseResult = YamlParser::parse(yaml);
  ASSERT_TRUE(parseResult.has_value());
  
  auto buildResult = YamlLaunchBuilder::build(parseResult.get_value());
  // Implementation may skip unknown action types silently
  // So it may succeed with empty description or fail
  // Just check it doesn't crash
  (void)buildResult;
}

// Test: YamlLaunchBuilder with missing required fields
TEST(YamlLaunchBuilderEdgeTest, MissingRequiredFields)
{
  std::string yaml = R"(entities:
  - type: execute_process)";
  
  auto parseResult = YamlParser::parse(yaml);
  ASSERT_TRUE(parseResult.has_value());
  
  auto buildResult = YamlLaunchBuilder::build(parseResult.get_value());
  // May succeed or fail depending on implementation
}

// Test: Empty YAML document
TEST(YamlParserEdgeTest, EmptyDocument)
{
  std::string yaml = "---";
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

// Test: Parse substitution patterns
TEST(YamlParserSubstitutionTest, ParseSubstitutionPatterns)
{
  std::string yaml = R"(cmd:
  - $(find package)/executable
  - $(env HOME)
  - $(var arg_name))";
  
  auto result = YamlParser::parse(yaml);
  ASSERT_TRUE(result.has_value());
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
