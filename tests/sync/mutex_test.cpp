#include "thread/sync/mutex.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

class MutexTest : public ::testing::Test {
protected:
  thread::sync::Mutex mutex;
};

TEST_F(MutexTest, BasicLockUnlock) {
  mutex.lock();
  mutex.unlock();
}

TEST_F(MutexTest, BasicLockable) {
  int value = 0;
  {
    std::lock_guard<thread::sync::Mutex> guard(mutex);
    value = 42;
  }
  EXPECT_EQ(value, 42);

  {
    std::unique_lock<thread::sync::Mutex> guard(mutex);
    value = 100;
    guard.unlock();
    EXPECT_FALSE(guard.owns_lock());
    guard.lock();
    EXPECT_TRUE(guard.owns_lock());
    value = 200;
  }
  EXPECT_EQ(value, 200);
}

TEST_F(MutexTest, MutualExclusion) {
  int counter = 0;
  const int num_threads = 10;
  const int increments = 10000;

  auto worker = [&]() {
    for (int i = 0; i < increments; ++i) {
      std::lock_guard<thread::sync::Mutex> guard(mutex);
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

TEST_F(MutexTest, HighContention) {
  std::vector<int> results(8, 0);
  const int iterations = 1000;

  auto worker = [&](int thread_id) {
    for (int i = 0; i < iterations; ++i) {
      std::lock_guard<thread::sync::Mutex> guard(mutex);
      results[thread_id]++;
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

TEST_F(MutexTest, MultipleSequentialLocks) {
  int value = 0;

  {
    std::lock_guard<thread::sync::Mutex> guard(mutex);
    value = 1;
  }

  {
    std::lock_guard<thread::sync::Mutex> guard(mutex);
    EXPECT_EQ(value, 1);
    value = 2;
  }

  {
    std::lock_guard<thread::sync::Mutex> guard(mutex);
    EXPECT_EQ(value, 2);
    value = 3;
  }

  EXPECT_EQ(value, 3);
}

TEST_F(MutexTest, RepeatedLockUnlock) {
  for (int i = 0; i < 1000; ++i) {
    mutex.lock();
    mutex.unlock();
  }
}

TEST_F(MutexTest, WaitWakeMechanism) {
  std::atomic<bool> start{false};
  std::atomic<int> entered{0};
  const int num_threads = 5;

  auto worker = [&]() {
    while (!start.load()) {
      std::this_thread::yield();
    }

    std::lock_guard<thread::sync::Mutex> guard(mutex);
    entered.fetch_add(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  };

  mutex.lock();

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  start.store(true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(entered.load(), 0);

  mutex.unlock();

  for (auto& t : threads) {
    t.join();
  }
  EXPECT_EQ(entered.load(), num_threads);
}

TEST_F(MutexTest, CriticalSectionProtection) {
  int shared_value = 0;
  std::atomic<bool> in_critical_section{false};
  std::atomic<bool> violation_detected{false};
  const int num_threads = 8;
  const int iterations = 1000;

  auto worker = [&]() {
    for (int i = 0; i < iterations; ++i) {
      mutex.lock();

      if (in_critical_section.exchange(true)) {
        violation_detected.store(true);
      }

      // Critical section
      int old_value = shared_value;
      std::this_thread::yield();
      shared_value = old_value + 1;

      in_critical_section.store(false);
      mutex.unlock();
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker);
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_FALSE(violation_detected.load())
    << "Multiple threads were in critical section simultaneously";
  EXPECT_EQ(shared_value, num_threads * iterations);
}

TEST_F(MutexTest, RapidLockUnlockCycles) {
  std::atomic<int> lock_count{0};
  const int num_threads = 4;
  const int iterations = 10000;

  auto worker = [&]() {
    for (int i = 0; i < iterations; ++i) {
      mutex.lock();
      lock_count.fetch_add(1);
      mutex.unlock();
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker);
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(lock_count.load(), num_threads * iterations);
}
