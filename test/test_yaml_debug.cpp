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
    
    auto result = YamlParser::Parse(yaml);
    if (result.HasError()) {
      std::cout << "Error: " << result.GetError().GetMessage() << std::endl;
    } else {
      std::cout << "Parsed successfully!" << std::endl;
      auto& val = result.GetValue();
      std::cout << "Type: " << static_cast<int>(val.GetType()) << std::endl;
      if (val.IsObject()) {
        std::cout << "Is object with " << val.AsObject().size() << " fields" << std::endl;
        for (const auto& field : val.AsObject()) {
          std::cout << "  " << field.first << " = " << field.second.AsString() << std::endl;
        }
      }
    }
  }
  
  // Test 2: Object with array
  std::cout << "\nTest 2: Object with array" << std::endl;
  {
    std::string yaml = "entities:\n  - type: test\n    name: hello\n";
    
    auto result = YamlParser::Parse(yaml);
    if (result.HasError()) {
      std::cout << "Error: " << result.GetError().GetMessage() << std::endl;
    } else {
      std::cout << "Parsed successfully!" << std::endl;
      auto& val = result.GetValue();
      std::cout << "Type: " << static_cast<int>(val.GetType()) << std::endl;
      if (val.IsObject()) {
        std::cout << "Is object with " << val.AsObject().size() << " fields" << std::endl;
      }
    }
  }
  
  // Test 3: Parse from file
  std::cout << "\nTest 3: Parse from file" << std::endl;
  {
    std::string filePath = "/home/bingdian/work/ros2/jazzy/src/ros2/launch_cpp/examples/test_simple.yaml";
    
    auto result = YamlParser::ParseFile(filePath);
    if (result.HasError()) {
      std::cout << "Error: " << result.GetError().GetMessage() << std::endl;
    } else {
      std::cout << "Parsed file successfully!" << std::endl;
      auto& val = result.GetValue();
      std::cout << "Root type: " << static_cast<int>(val.GetType()) << std::endl;
      
      if (val.IsObject()) {
        std::cout << "Root object has " << val.AsObject().size() << " fields" << std::endl;
        
        auto entities = val.AsObject().find("entities");
        if (entities != val.AsObject().end()) {
          std::cout << "Found 'entities' field" << std::endl;
          if (entities->second.IsArray()) {
            std::cout << "Entities is array with " << entities->second.AsArray().size() << " items" << std::endl;
          } else {
            std::cout << "Entities is NOT an array, type: " << static_cast<int>(entities->second.GetType()) << std::endl;
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
