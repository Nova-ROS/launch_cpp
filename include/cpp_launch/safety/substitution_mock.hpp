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


/**
 * @file substitution_mock.hpp
 * @brief Minimal mock of Substitution for testing
 *
 * This provides a minimal implementation to allow compilation
 * without requiring the full cpp_launch headers.
 */

#ifndef SAFETY_PROJECT_SUBSTITUTION_MOCK_HPP_
#define SAFETY_PROJECT_SUBSTITUTION_MOCK_HPP_

#include <string>

namespace cpp_launch {

// Forward declaration
class LaunchContext;

// Minimal Substitution base class for testing
class Substitution {
public:
    Substitution() = default;
    virtual ~Substitution() = default;

    Substitution(const Substitution&) = default;
    Substitution& operator=(const Substitution&) = default;
    Substitution(Substitution&&) = default;
    Substitution& operator=(Substitution&&) = default;

    virtual std::string Perform(const LaunchContext& context) const = 0;

    std::string operator()(const LaunchContext& context) const {
        return Perform(context);
    }
};

// Minimal LaunchContext for testing
class LaunchContext {
public:
    // Empty context for testing
};

}  // namespace cpp_launch

#endif  // SAFETY_PROJECT_SUBSTITUTION_MOCK_HPP_
