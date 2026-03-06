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

#include "cpp_launch/thread_pool.hpp"
#include <algorithm>

namespace cpp_launch
{

ThreadPool::ThreadPool(std::size_t numThreads)
  : status_(ThreadPoolStatus::kRunning)
{
  threads_.reserve(numThreads);
  
  for (std::size_t i = 0U; i < numThreads; ++i)
  {
    threads_.emplace_back(&ThreadPool::WorkerThread, this);
  }
}

ThreadPool::~ThreadPool() noexcept
{
  Shutdown();
}

Error ThreadPool::Submit(std::function<void()> task)
{
  ThreadPoolStatus currentStatus = status_.load(std::memory_order_acquire);
  
  if (currentStatus != ThreadPoolStatus::kRunning)
  {
    return Error(ErrorCode::kInternalError, "Thread pool is not running");
  }
  
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    taskQueue_.push(std::move(task));
  }
  
  condition_.notify_one();
  return Error();
}

void ThreadPool::Shutdown()
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

void ThreadPool::WorkerThread()
{
  while (true)
  {
    std::function<void()> task;
    
    {
      std::unique_lock<std::mutex> lock(queueMutex_);
      
      condition_.wait(lock, [this]() {
        return !taskQueue_.empty() || 
               status_.load(std::memory_order_acquire) != ThreadPoolStatus::kRunning;
      });
      
      if (status_.load(std::memory_order_acquire) != ThreadPoolStatus::kRunning &&
          taskQueue_.empty())
      {
        return;
      }
      
      if (!taskQueue_.empty())
      {
        task = std::move(taskQueue_.front());
        taskQueue_.pop();
      }
    }
    
    if (task)
    {
      task();
    }
  }
}

}  // namespace cpp_launch
