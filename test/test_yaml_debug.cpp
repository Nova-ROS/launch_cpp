// Test YAML parser
#include <iostream>
#include <fstream>
#include <sstream>
#include "launch_cpp/yaml_parser.hpp"

using namespace launch_cpp;

int main()
{
  std::cout << "=== YAML Parser Test ===" << std::endl;
  
  // Test 1: Very simple object (no comments, no empty lines)
  std::cout << "\nTest 1: Very simple object" << std::endl;
  {
    std::string yaml = "key: value\n";
    
    auto result = YamlParser::parse(yaml);
    if (result.has_error()) {
      std::cout << "Error: " << result.get_error().get_message() << std::endl;
    } else {
      std::cout << "Parsed successfully!" << std::endl;
      auto& val = result.get_value();
      std::cout << "Type: " << static_cast<int>(val.get_type()) << std::endl;
      if (val.is_object()) {
        std::cout << "Is object with " << val.as_object().size() << " fields" << std::endl;
        for (const auto& field : val.as_object()) {
          std::cout << "  " << field.first << " = " << field.second.as_string() << std::endl;
        }
      }
    }
  }
  
  // Test 2: Object with array
  std::cout << "\nTest 2: Object with array" << std::endl;
  {
    std::string yaml = "entities:\n  - type: test\n    name: hello\n";
    
    auto result = YamlParser::parse(yaml);
    if (result.has_error()) {
      std::cout << "Error: " << result.get_error().get_message() << std::endl;
    } else {
      std::cout << "Parsed successfully!" << std::endl;
      auto& val = result.get_value();
      std::cout << "Type: " << static_cast<int>(val.get_type()) << std::endl;
      if (val.is_object()) {
        std::cout << "Is object with " << val.as_object().size() << " fields" << std::endl;
      }
    }
  }

  // Test 3: Parse from file
  std::cout << "\nTest 3: Parse from file" << std::endl;
  {
    std::string file_path = "/home/bingdian/work/ros2/jazzy/src/ros2/launch_cpp/examples/test_simple.yaml";

    auto result = YamlParser::parse_file(file_path);
    if (result.has_error()) {
      std::cout << "Error: " << result.get_error().get_message() << std::endl;
    } else {
      std::cout << "Parsed file successfully!" << std::endl;
      auto& val = result.get_value();
      std::cout << "Root type: " << static_cast<int>(val.get_type()) << std::endl;
      
      if (val.is_object()) {
        std::cout << "Root object has " << val.as_object().size() << " fields" << std::endl;
        
        auto entities = val.as_object().find("entities");
        if (entities != val.as_object().end()) {
          std::cout << "Found 'entities' field" << std::endl;
          if (entities->second.is_array()) {
            std::cout << "Entities is array with " << entities->second.as_array().size() << " items" << std::endl;
          } else {
            std::cout << "Entities is NOT an array, type: " << static_cast<int>(entities->second.get_type()) << std::endl;
          }
        } else {
          std::cout << "'entities' field not found" << std::endl;
        }
      } else {
        std::cout << "Root is NOT an object" << std::endl;
      }
    }
  }
  
  std::cout << "\n=== Test Complete ===" << std::endl;
  return 0;
}
