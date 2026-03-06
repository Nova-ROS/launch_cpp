// Copyright 2024 Example Author
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

#ifndef CPP_LAUNCH__SINGLETON_HPP_
#define CPP_LAUNCH__SINGLETON_HPP_

// AUTOSAR C++14 Compliant Singleton Pattern
// Non-intrusive, thread-safe implementation

#include <mutex>
#include <atomic>

namespace cpp_launch
{

// AUTOSAR C++14: A18-5-2 - Do not use exceptions
// AUTOSAR C++14: M5-0-3 - Use explicit type conversions
template<typename T>
class Singleton final
{
 public:
  // AUTOSAR C++14: A10-3-2 - Declare delete functions public
  Singleton() = delete;
  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;
  Singleton(Singleton&&) = delete;
  Singleton& operator=(Singleton&&) = delete;
  ~Singleton() = delete;
  
  // AUTOSAR C++14: M0-1-9 - Declare functions as noexcept
  static T& Instance() noexcept
  {
    // AUTOSAR C++14: Double-checked locking pattern
    T* instance = instance_.load(std::memory_order_acquire);
    
    if (instance == nullptr)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      instance = instance_.load(std::memory_order_relaxed);
      
      if (instance == nullptr)
      {
        // AUTOSAR C++14: A18-5-2 - Use placement new
        static char buffer[sizeof(T)];
        instance = new (buffer) T();
        instance_.store(instance, std::memory_order_release);
      }
    }
    
    return *instance;
  }
  
  // AUTOSAR C++14: Check if initialized
  static bool IsInitialized() noexcept
  {
    return instance_.load(std::memory_order_acquire) != nullptr;
  }
  
 private:
  static std::atomic<T*> instance_;
  static std::mutex mutex_;
};

// AUTOSAR C++14: A2-11-1 - Define static members outside class
template<typename T>
std::atomic<T*> Singleton<T>::instance_{nullptr};

template<typename T>
std::mutex Singleton<T>::mutex_;

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__SINGLETON_HPP_
