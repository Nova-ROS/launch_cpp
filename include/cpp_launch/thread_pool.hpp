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

#ifndef CPP_LAUNCH__THREAD_POOL_HPP_
#define CPP_LAUNCH__THREAD_POOL_HPP_

// AUTOSAR C++14 Compliant Thread Pool
// Fixed size, task queue based

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>
#include <functional>
#include "cpp_launch/error_code.hpp"

namespace cpp_launch
{

// AUTOSAR C++14: A7-2-4 - Use enum class for enumeration types
enum class ThreadPoolStatus : std::int32_t
{
  kRunning = 0,
  kShuttingDown = 1,
  kStopped = 2
};

// AUTOSAR C++14: Forward declaration
class Task;

// AUTOSAR C++14: M0-1-9 - Declare functions as noexcept where appropriate
class ThreadPool final
{
 public:
  // AUTOSAR C++14: A12-1-1 - Use member initialization list
  explicit ThreadPool(std::size_t numThreads);
  
  // AUTOSAR C++14: A12-8-4 - Declare destructor noexcept
  ~ThreadPool() noexcept;
  
  // AUTOSAR C++14: A10-3-2 - Declare special functions
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;
  
  // AUTOSAR C++14: M0-1-9 - Declare functions as noexcept
  Error Submit(std::function<void()> task);
  
  void Shutdown();
  
  std::size_t GetThreadCount() const noexcept { return threads_.size(); }
  
  ThreadPoolStatus GetStatus() const noexcept
  {
    return status_.load(std::memory_order_acquire);
  }
  
 private:
  void WorkerThread();
  
  std::vector<std::thread> threads_;
  std::queue<std::function<void()>> taskQueue_;
  std::mutex queueMutex_;
  std::condition_variable condition_;
  std::atomic<ThreadPoolStatus> status_;
};

}  // namespace cpp_launch

#endif  // CPP_LAUNCH__THREAD_POOL_HPP_
