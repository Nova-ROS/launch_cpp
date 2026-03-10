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


#include "launch_cpp/thread_pool.hpp"
#include <algorithm>

namespace launch_cpp
{

ThreadPool::ThreadPool(std::size_t num_threads)
  : status_(ThreadPoolStatus::kRunning)
{
  threads_.reserve(num_threads);

  for (std::size_t i = 0U; i < num_threads; ++i)
  {
    threads_.emplace_back(&ThreadPool::worker_thread, this);
  }
}

ThreadPool::~ThreadPool() noexcept
{
  shutdown();
}

Error ThreadPool::submit(std::function<void()> task)
{
  ThreadPoolStatus current_status = status_.load(std::memory_order_acquire);

  if (current_status != ThreadPoolStatus::kRunning)
  {
    return Error(ErrorCode::kInternalError, "Thread pool is not running");
  }

  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    task_queue_.push(std::move(task));
  }

  condition_.notify_one();
  return Error();
}

void ThreadPool::shutdown()
{
  ThreadPoolStatus expected = ThreadPoolStatus::kRunning;

  if (!status_.compare_exchange_strong(
        expected,
        ThreadPoolStatus::kShuttingDown,
        std::memory_order_release,
        std::memory_order_relaxed))
  {
    return;
  }

  condition_.notify_all();

  for (std::thread& thread : threads_)
  {
    if (thread.joinable())
    {
      thread.join();
    }
  }

  status_.store(ThreadPoolStatus::kStopped, std::memory_order_release);
}

void ThreadPool::worker_thread()
{
  while (true)
  {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);

      condition_.wait(lock, [this]() {
        return !task_queue_.empty() ||
               status_.load(std::memory_order_acquire) != ThreadPoolStatus::kRunning;
      });

      if (status_.load(std::memory_order_acquire) != ThreadPoolStatus::kRunning &&
          task_queue_.empty())
      {
        return;
      }

      if (!task_queue_.empty())
      {
        task = std::move(task_queue_.front());
        task_queue_.pop();
      }
    }

    if (task)
    {
      task();
    }
  }
}

}  // namespace launch_cpp
