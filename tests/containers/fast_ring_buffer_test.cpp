#include <gtest/gtest.h>

#include <algorithm>
#include <common/containers/ring_buffer.hpp>
#include <numeric>
#include <thread>
#include <vector>

using common::containers::FastRingBuffer;

class FastRingBufferTest : public ::testing::Test {
protected:
  void SetUp() override {
  }

  void TearDown() override {
  }
};

TEST_F(FastRingBufferTest, BasicPushPop) {
  FastRingBuffer<int> buffer(10);

  EXPECT_TRUE(buffer.Push(42));
  auto val = buffer.Pop();
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(val.value(), 42);
}

TEST_F(FastRingBufferTest, EmptyPop) {
  FastRingBuffer<int> buffer(10);

  auto val = buffer.Pop();
  EXPECT_FALSE(val.has_value());
}

TEST_F(FastRingBufferTest, FillBuffer) {
  const size_t capacity = 10;
  FastRingBuffer<int> buffer(capacity);

  for (size_t i = 0; i < capacity - 1; ++i) {
    EXPECT_TRUE(buffer.Push(static_cast<int>(i)));
  }

  EXPECT_FALSE(buffer.Push(999));
}

TEST_F(FastRingBufferTest, PushPopSequence) {
  FastRingBuffer<int> buffer(5);

  EXPECT_TRUE(buffer.Push(1));
  EXPECT_TRUE(buffer.Push(2));
  EXPECT_TRUE(buffer.Push(3));

  auto val1 = buffer.Pop();
  ASSERT_TRUE(val1.has_value());
  EXPECT_EQ(val1.value(), 1);

  auto val2 = buffer.Pop();
  ASSERT_TRUE(val2.has_value());
  EXPECT_EQ(val2.value(), 2);

  EXPECT_TRUE(buffer.Push(4));
  EXPECT_TRUE(buffer.Push(5));

  auto val3 = buffer.Pop();
  ASSERT_TRUE(val3.has_value());
  EXPECT_EQ(val3.value(), 3);
}

TEST_F(FastRingBufferTest, WrapAround) {
  const size_t capacity = 5;
  FastRingBuffer<int> buffer(capacity);

  for (size_t i = 0; i < capacity - 1; ++i) {
    EXPECT_TRUE(buffer.Push(static_cast<int>(i)));
  }

  for (size_t i = 0; i < capacity - 1; ++i) {
    auto val = buffer.Pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), static_cast<int>(i));
  }

  for (size_t i = 0; i < capacity - 1; ++i) {
    EXPECT_TRUE(buffer.Push(static_cast<int>(i + 100)));
  }

  for (size_t i = 0; i < capacity - 1; ++i) {
    auto val = buffer.Pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), static_cast<int>(i + 100));
  }
}

TEST_F(FastRingBufferTest, AlternatingPushPop) {
  FastRingBuffer<int> buffer(10);

  for (int i = 0; i < 100; ++i) {
    EXPECT_TRUE(buffer.Push(i));
    auto val = buffer.Pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), i);
  }
}

TEST_F(FastRingBufferTest, MoveSemantics) {
  struct MoveOnly {
    int value;
    MoveOnly(int v) : value(v) {
    }
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;
  };

  FastRingBuffer<MoveOnly> buffer(10);

  EXPECT_TRUE(buffer.Push(MoveOnly(42)));
  auto val = buffer.Pop();
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(val.value().value, 42);
}

TEST_F(FastRingBufferTest, CacheOptimization) {
  const size_t num_items = 100000;
  const size_t capacity = 128;
  FastRingBuffer<size_t> buffer(capacity);

  std::atomic<bool> producer_done{false};
  std::vector<size_t> consumed;
  consumed.reserve(num_items);

  std::thread producer([&]() {
    for (size_t i = 0; i < num_items; ++i) {
      while (!buffer.Push(i)) {
        std::this_thread::yield();
      }
    }
    producer_done.store(true);
  });

  std::thread consumer([&]() {
    while (!producer_done.load() || consumed.size() < num_items) {
      auto val = buffer.Pop();
      if (val.has_value()) {
        consumed.push_back(val.value());
      } else {
        std::this_thread::yield();
      }
    }
  });

  producer.join();
  consumer.join();

  ASSERT_EQ(consumed.size(), num_items);
  for (size_t i = 0; i < num_items; ++i) {
    EXPECT_EQ(consumed[i], i);
  }
}

TEST_F(FastRingBufferTest, HighContentionSPSC) {
  const size_t num_items = 50000;
  const size_t capacity = 16;
  FastRingBuffer<size_t> buffer(capacity);

  std::atomic<bool> producer_done{false};
  std::atomic<size_t> sum_consumed{0};

  std::thread producer([&]() {
    for (size_t i = 0; i < num_items; ++i) {
      while (!buffer.Push(i)) {
        std::this_thread::yield();
      }
    }
    producer_done.store(true);
  });

  std::thread consumer([&]() {
    size_t count = 0;
    while (count < num_items) {
      auto val = buffer.Pop();
      if (val.has_value()) {
        sum_consumed.fetch_add(val.value());
        count++;
      } else {
        std::this_thread::yield();
      }
    }
  });

  producer.join();
  consumer.join();

  size_t expected_sum = (num_items * (num_items - 1)) / 2;
  EXPECT_EQ(sum_consumed.load(), expected_sum);
}

TEST_F(FastRingBufferTest, BurstProduceConsume) {
  const size_t capacity = 64;
  const size_t burst_size = 30;
  const size_t num_bursts = 1000;
  FastRingBuffer<size_t> buffer(capacity);

  std::atomic<bool> producer_done{false};
  std::vector<size_t> consumed;
  consumed.reserve(burst_size * num_bursts);

  std::thread producer([&]() {
    for (size_t burst = 0; burst < num_bursts; ++burst) {
      for (size_t i = 0; i < burst_size; ++i) {
        while (!buffer.Push(burst * burst_size + i)) {
          std::this_thread::yield();
        }
      }
    }
    producer_done.store(true);
  });

  std::thread consumer([&]() {
    while (!producer_done.load() || consumed.size() < burst_size * num_bursts) {
      auto val = buffer.Pop();
      if (val.has_value()) {
        consumed.push_back(val.value());
      } else {
        std::this_thread::yield();
      }
    }
  });

  producer.join();
  consumer.join();

  ASSERT_EQ(consumed.size(), burst_size * num_bursts);
  for (size_t i = 0; i < burst_size * num_bursts; ++i) {
    EXPECT_EQ(consumed[i], i);
  }
}

TEST_F(FastRingBufferTest, StressTestWithDelays) {
  const size_t num_items = 10000;
  const size_t capacity = 32;
  FastRingBuffer<size_t> buffer(capacity);

  std::atomic<bool> producer_done{false};
  std::vector<size_t> consumed;
  consumed.reserve(num_items);

  std::thread producer([&]() {
    for (size_t i = 0; i < num_items; ++i) {
      while (!buffer.Push(i)) {
        std::this_thread::yield();
      }
      if (i % 100 == 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
      }
    }
    producer_done.store(true);
  });

  std::thread consumer([&]() {
    while (!producer_done.load() || consumed.size() < num_items) {
      auto val = buffer.Pop();
      if (val.has_value()) {
        consumed.push_back(val.value());
      } else {
        std::this_thread::yield();
      }
      if (consumed.size() % 100 == 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
      }
    }
  });

  producer.join();
  consumer.join();

  ASSERT_EQ(consumed.size(), num_items);
  for (size_t i = 0; i < num_items; ++i) {
    EXPECT_EQ(consumed[i], i);
  }
}
