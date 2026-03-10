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


#ifndef LAUNCH_CPP__THREAD_POOL_HPP_
#define LAUNCH_CPP__THREAD_POOL_HPP_

// AUTOSAR C++14 Compliant Thread Pool
// Fixed size, task queue based

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>
#include <functional>
#include "launch_cpp/error_code.hpp"

namespace launch_cpp
{

// AUTOSAR C++14: A7-2-4 - Use enum class for enumeration types
enum class ThreadPoolStatus : std::int32_t
{
  K_RUNNING = 0,
  K_SHUTTING_DOWN = 1,
  K_STOPPED = 2
};

// AUTOSAR C++14: Forward declaration
class Task;

// AUTOSAR C++14: M0-1-9 - Declare functions as noexcept where appropriate
class ThreadPool final
{
 public:
  // AUTOSAR C++14: A12-1-1 - Use member initialization list
  explicit ThreadPool(std::size_t num_threads);

  // AUTOSAR C++14: A12-8-4 - Declare destructor noexcept
  ~ThreadPool() noexcept;

  // AUTOSAR C++14: A10-3-2 - Declare special functions
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  // AUTOSAR C++14: M0-1-9 - Declare functions as noexcept
  Error submit(std::function<void()> task);

  void shutdown();

  std::size_t get_thread_count() const noexcept { return threads_.size(); }

  ThreadPoolStatus get_status() const noexcept
  {
    return status_.load(std::memory_order_acquire);
  }

 private:
  void worker_thread();

  std::vector<std::thread> threads_;
  std::queue<std::function<void()>> task_queue_;
  std::mutex queue_mutex_;
  std::condition_variable condition_;
  std::atomic<ThreadPoolStatus> status_;
};

}  // namespace launch_cpp

#endif  // LAUNCH_CPP__THREAD_POOL_HPP_
