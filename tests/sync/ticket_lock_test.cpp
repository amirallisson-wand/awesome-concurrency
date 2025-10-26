#include "thread/sync/ticket_lock.hpp"

#include <gtest/gtest.h>

#include <mutex>
#include <thread>
#include <vector>

class TicketLockTest : public ::testing::Test {
protected:
  thread::sync::TicketLock lock;
};

TEST_F(TicketLockTest, BasicLockUnlock) {
  lock.lock();
  lock.unlock();
}

TEST_F(TicketLockTest, TryLockSuccess) {
  EXPECT_TRUE(lock.try_lock());
  lock.unlock();
}

TEST_F(TicketLockTest, TryLockFailure) {
  lock.lock();
  EXPECT_FALSE(lock.try_lock());
  lock.unlock();
}

TEST_F(TicketLockTest, BasicLockable) {
  int value = 0;
  {
    std::lock_guard<thread::sync::TicketLock> guard(lock);
    value = 42;
  }
  EXPECT_EQ(value, 42);

  {
    std::unique_lock<thread::sync::TicketLock> guard(lock);
    value = 100;
    guard.unlock();
    EXPECT_FALSE(guard.owns_lock());
    guard.lock();
    EXPECT_TRUE(guard.owns_lock());
    value = 200;
  }
  EXPECT_EQ(value, 200);
}

TEST_F(TicketLockTest, MutualExclusion) {
  int counter = 0;
  const int num_threads = 10;
  const int increments = 10000;

  auto worker = [&]() {
    for (int i = 0; i < increments; ++i) {
      std::lock_guard<thread::sync::TicketLock> guard(lock);
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

TEST_F(TicketLockTest, HighContention) {
  std::vector<int> results(8, 0);
  const int iterations = 1000;

  auto worker = [&](int thread_id) {
    for (int i = 0; i < iterations; ++i) {
      std::lock_guard<thread::sync::TicketLock> guard(lock);
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

TEST_F(TicketLockTest, MultipleSequentialLocks) {
  int value = 0;

  {
    std::lock_guard<thread::sync::TicketLock> guard(lock);
    value = 1;
  }

  {
    std::lock_guard<thread::sync::TicketLock> guard(lock);
    EXPECT_EQ(value, 1);
    value = 2;
  }

  {
    std::lock_guard<thread::sync::TicketLock> guard(lock);
    EXPECT_EQ(value, 2);
    value = 3;
  }

  EXPECT_EQ(value, 3);
}

TEST_F(TicketLockTest, TryLockInContention) {
  std::atomic<int> successful_locks{0};
  const int num_threads = 4;

  auto worker = [&]() {
    for (int i = 0; i < 100; ++i) {
      if (lock.try_lock()) {
        ++successful_locks;
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        lock.unlock();
      }
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker);
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_GT(successful_locks.load(), 0);
}
