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


// Comprehensive Thread Pool Tests
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <future>
#include "launch_cpp/thread_pool.hpp"

using namespace launch_cpp;

// Test: Thread pool creation with different sizes
TEST(ThreadPoolTest, Creation)
{
  // Test with 1 thread
  {
    ThreadPool pool(1);
    EXPECT_EQ(pool.GetThreadCount(), 1U);
  }
  
  // Test with 4 threads
  {
    ThreadPool pool(4);
    EXPECT_EQ(pool.GetThreadCount(), 4U);
  }
  
  // Test with 8 threads
  {
    ThreadPool pool(8);
    EXPECT_EQ(pool.GetThreadCount(), 8U);
  }
}

// Test: Single task execution
TEST(ThreadPoolTest, SingleTask)
{
  ThreadPool pool(2);
  std::atomic<int> counter(0);
  
  Error err = pool.Submit([&counter]() { counter++; });
  EXPECT_TRUE(err.IsSuccess());
  
  pool.Shutdown();
  EXPECT_EQ(counter.load(), 1);
}

// Test: Multiple task execution
TEST(ThreadPoolTest, MultipleTasks)
{
  ThreadPool pool(4);
  std::atomic<int> counter(0);
  const int num_tasks = 100;
  
  for (int i = 0; i < num_tasks; ++i) {
    Error err = pool.Submit([&counter]() {
      counter++;
    });
    EXPECT_TRUE(err.IsSuccess());
  }
  
  pool.Shutdown();
  EXPECT_EQ(counter.load(), num_tasks);
}

// Test: Concurrent task execution
TEST(ThreadPoolTest, ConcurrentExecution)
{
  ThreadPool pool(4);
  std::atomic<int> concurrent_count(0);
  std::atomic<int> max_concurrent(0);
  const int num_tasks = 20;
  
  for (int i = 0; i < num_tasks; ++i) {
    pool.Submit([&concurrent_count, &max_concurrent]() {
      int current = ++concurrent_count;
      int prev_max = max_concurrent.load();
      while (current > prev_max && !max_concurrent.compare_exchange_weak(prev_max, current)) {}
      
      // Simulate some work
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      
      concurrent_count--;
    });
  }
  
  pool.Shutdown();
  
  // Should have had some concurrent execution
  EXPECT_GT(max_concurrent.load(), 1);
  EXPECT_LE(max_concurrent.load(), 4);  // Limited by thread count
}

// Test: Task with return value simulation
TEST(ThreadPoolTest, TaskWithResult)
{
  ThreadPool pool(2);
  std::atomic<int> sum(0);
  const int num_tasks = 10;
  
  // Use promise/future pattern with shared ownership
  std::vector<std::shared_ptr<std::promise<int>>> promises;
  std::vector<std::future<int>> futures;
  
  for (int i = 0; i < num_tasks; ++i) {
    auto promise = std::make_shared<std::promise<int>>();
    promises.push_back(promise);
    futures.push_back(promise->get_future());
    
    pool.Submit([promise, i]() {
      promise->set_value(i * i);
    });
  }
  
  pool.Shutdown();
  
  // Verify results
  for (int i = 0; i < num_tasks; ++i) {
    EXPECT_EQ(futures[i].get(), i * i);
  }
}

// Test: Shutdown behavior
TEST(ThreadPoolTest, Shutdown)
{
  ThreadPool pool(2);
  std::atomic<bool> task_completed(false);
  
  pool.Submit([&task_completed]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    task_completed = true;
  });
  
  pool.Shutdown();
  
  EXPECT_TRUE(task_completed.load());
}

// Test: Submit after shutdown should fail
TEST(ThreadPoolTest, SubmitAfterShutdown)
{
  ThreadPool pool(2);
  pool.Shutdown();
  
  Error err = pool.Submit([]() {});
  EXPECT_TRUE(err.IsError());
}

// Test: Multiple shutdown calls (should be safe)
TEST(ThreadPoolTest, MultipleShutdowns)
{
  ThreadPool pool(2);
  
  // Should not crash or throw
  pool.Shutdown();
  pool.Shutdown();
  pool.Shutdown();
}

// Test: Multiple tasks with some error conditions
TEST(ThreadPoolTest, TaskErrorHandling)
{
  ThreadPool pool(2);
  std::atomic<int> counter(0);
  std::atomic<int> error_count(0);
  
  // Submit tasks - some succeed, some "fail" (but don't throw)
  for (int i = 0; i < 10; ++i) {
    pool.Submit([&counter, &error_count, i]() {
      if (i % 3 == 0) {
        // Simulate error condition
        error_count++;
      } else {
        counter++;
      }
    });
  }
  
  pool.Shutdown();
  
  // All tasks should complete
  EXPECT_EQ(error_count.load(), 4);  // 4 "error" tasks (0, 3, 6, 9)
  EXPECT_EQ(counter.load(), 6);  // 6 successful tasks (10 - 4)
}

// Test: High load stress test
TEST(ThreadPoolTest, HighLoadStress)
{
  ThreadPool pool(4);
  std::atomic<int> counter(0);
  const int num_tasks = 1000;
  
  for (int i = 0; i < num_tasks; ++i) {
    pool.Submit([&counter]() {
      counter++;
    });
  }
  
  pool.Shutdown();
  EXPECT_EQ(counter.load(), num_tasks);
}

// Test: Task order is not guaranteed (concurrent execution)
TEST(ThreadPoolTest, NoOrderGuarantee)
{
  ThreadPool pool(4);
  std::vector<int> order;
  std::mutex order_mutex;
  
  for (int i = 0; i < 10; ++i) {
    pool.Submit([i, &order, &order_mutex]() {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
      std::lock_guard<std::mutex> lock(order_mutex);
      order.push_back(i);
    });
  }
  
  pool.Shutdown();
  
  EXPECT_EQ(order.size(), 10U);
  // Order is not guaranteed to be 0,1,2,...
  bool sequential = true;
  for (size_t i = 0; i < order.size(); ++i) {
    if (order[i] != static_cast<int>(i)) {
      sequential = false;
      break;
    }
  }
  // Either sequential or not, both are valid
  (void)sequential;
}

// Test: Empty pool (1 thread)
TEST(ThreadPoolTest, SingleThreadPool)
{
  ThreadPool pool(1);
  std::atomic<int> counter(0);
  const int num_tasks = 10;
  
  for (int i = 0; i < num_tasks; ++i) {
    pool.Submit([&counter]() {
      counter++;
    });
  }
  
  pool.Shutdown();
  EXPECT_EQ(counter.load(), num_tasks);
}

// Test: Large thread pool
TEST(ThreadPoolTest, LargeThreadPool)
{
  ThreadPool pool(16);
  std::atomic<int> counter(0);
  const int num_tasks = 32;
  
  for (int i = 0; i < num_tasks; ++i) {
    pool.Submit([&counter]() {
      counter++;
    });
  }
  
  pool.Shutdown();
  EXPECT_EQ(counter.load(), num_tasks);
}

// Test: Submit from multiple threads
TEST(ThreadPoolTest, MultiThreadedSubmit)
{
  ThreadPool pool(4);
  std::atomic<int> counter(0);
  const int num_threads = 4;
  const int tasks_per_thread = 25;
  
  std::vector<std::thread> threads;
  
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&pool, &counter]() {
      for (int i = 0; i < tasks_per_thread; ++i) {
        pool.Submit([&counter]() {
          counter++;
        });
      }
    });
  }
  
  for (auto& t : threads) {
    t.join();
  }
  
  pool.Shutdown();
  EXPECT_EQ(counter.load(), num_threads * tasks_per_thread);
}

// Test: Recursive task submission
TEST(ThreadPoolTest, RecursiveSubmit)
{
  ThreadPool pool(4);
  std::atomic<int> counter(0);
  const int depth = 3;
  
  std::function<void(int)> recursive_task = [&](int remaining) {
    counter++;
    if (remaining > 0) {
      pool.Submit([&recursive_task, remaining]() {
        recursive_task(remaining - 1);
      });
      pool.Submit([&recursive_task, remaining]() {
        recursive_task(remaining - 1);
      });
    }
  };
  
  pool.Submit([&recursive_task]() {
    recursive_task(depth);
  });
  
  // Wait a bit for recursive tasks to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  
  pool.Shutdown();
  
  // With depth 3: 1 + 2 + 4 + 8 = 15
  EXPECT_EQ(counter.load(), 15);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
