#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "thread/sync/mcs_spinlock.hpp"

class MCSSpinlockTest : public ::testing::Test {
protected:
  thread::sync::QueueSpinLock spinlock;
};

TEST_F(MCSSpinlockTest, BasicLockUnlock) {
  // Should be able to acquire and release lock without issues
  thread::sync::QueueSpinLock::Guard guard{spinlock};
  // Critical section
}

TEST_F(MCSSpinlockTest, SequentialLocks) {
  int value = 0;

  {
    thread::sync::QueueSpinLock::Guard guard1{spinlock};
    value = 1;
  }

  {
    thread::sync::QueueSpinLock::Guard guard2{spinlock};
    EXPECT_EQ(value, 1);
    value = 2;
  }

  EXPECT_EQ(value, 2);
}

TEST_F(MCSSpinlockTest, MutualExclusion) {
  int counter = 0;
  const int num_threads = 10;
  const int increments = 10000;

  auto worker = [&]() {
    for (int i = 0; i < increments; ++i) {
      thread::sync::QueueSpinLock::Guard guard{spinlock};
      ++counter;
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker);
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(counter, num_threads * increments);
}

TEST_F(MCSSpinlockTest, HighContention) {
  std::vector<int> results(8, 0);
  const int iterations = 1000;

  auto worker = [&](int thread_id) {
    for (int i = 0; i < iterations; ++i) {
      thread::sync::QueueSpinLock::Guard guard{spinlock};
      results[thread_id]++;
      // Simulate some work
      volatile int x = 0;
      for (int j = 0; j < 10; ++j) {
        x += j;
      }
    }
  };

  std::vector<std::thread> threads;
  for (size_t i = 0; i < results.size(); ++i) {
    threads.emplace_back(worker, i);
  }

  for (auto& t : threads) {
    t.join();
  }

  for (size_t i = 0; i < results.size(); ++i) {
    EXPECT_EQ(results[i], iterations) << "Thread " << i << " failed";
  }
}

TEST_F(MCSSpinlockTest, MultipleThreadsComplete) {
  std::vector<int> acquisition_order;
  const int num_threads = 5;

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([this, &acquisition_order, i]() {
      thread::sync::QueueSpinLock::Guard guard{spinlock};
      acquisition_order.push_back(i);
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(acquisition_order.size(), num_threads);
}
