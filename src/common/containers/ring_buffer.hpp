#pragma once

#include <atomic>
#include <optional>
#include <vector>

namespace common::containers {

#ifdef __cpp_lib_hardware_interference_size
constexpr size_t kCacheLineSize = std::hardware_destructive_interference_size;
#else
constexpr size_t kCacheLineSize = 64;
#endif

template <typename T>
class RingBuffer {
public:
  RingBuffer(size_t capacity) : capacity_(capacity) {
    data_.reserve(capacity_);
  }

  bool Push(T val) {
    // can use relaxed due to Modification Ordering guarantee
    const auto currentWriteIdx = writeIdx_.load(std::memory_order_relaxed);
    auto nextWriteIdx = (currentWriteIdx + 1) % capacity_;
    if (nextWriteIdx == readIdx_.load(std::memory_order_acquire)) {
      return false;
    }

    // lazy construction
    if (currentWriteIdx >= data_.size()) {
      data_.emplace_back(std::move(val));
    } else {
      data_[currentWriteIdx] = std::move(val);
    }

    writeIdx_.store(nextWriteIdx, std::memory_order_release);
    return true;
  }

  std::optional<T> Pop() {
    // can use relaxed due to Modification Ordering guarantee
    auto const readIdx = readIdx_.load(std::memory_order_relaxed);
    if (readIdx == writeIdx_.load(std::memory_order_acquire)) {
      return std::nullopt;
    }
    auto val = std::move(data_[readIdx]);
    auto nextReadIdx = (readIdx + 1) % capacity_;
    readIdx_.store(nextReadIdx, std::memory_order_release);
    return val;
  }

private:
  std::vector<T> data_{};
  alignas(kCacheLineSize) size_t capacity_{0};
  alignas(kCacheLineSize) std::atomic<size_t> readIdx_{0};
  alignas(kCacheLineSize) std::atomic<size_t> writeIdx_{0};
};

template <typename T>
class FastRingBuffer {
public:
  FastRingBuffer(size_t capacity) : capacity_(capacity) {
    data_.reserve(capacity);
  }

  bool Push(T val) {
    // can use relaxed due to Modification Ordering guarantee
    auto const currentWriteIdx = writeIdx_.load(std::memory_order_relaxed);
    auto nextWriteIdx = (currentWriteIdx + 1) % capacity_;
    if (nextWriteIdx == readIdxCached_) {
      readIdxCached_ = readIdx_.load(std::memory_order_acquire);
      if (nextWriteIdx == readIdxCached_) {
        return false;
      }
    }

    // lazy construction
    if (currentWriteIdx >= data_.size()) {
      data_.emplace_back(std::move(val));
    } else {
      data_[currentWriteIdx] = std::move(val);
    }
    writeIdx_.store(nextWriteIdx, std::memory_order_release);
    return true;
  }

  std::optional<T> Pop() {
    // can use relaxed due to Modification Ordering guarantee
    auto const readIdx = readIdx_.load(std::memory_order_relaxed);
    if (readIdx == writeIdxCached_) {
      writeIdxCached_ = writeIdx_.load(std::memory_order_acquire);
      if (readIdx == writeIdxCached_) {
        return std::nullopt;
      }
    }
    auto val = std::move(data_[readIdx]);
    auto nextReadIdx = (readIdx + 1) % capacity_;
    readIdx_.store(nextReadIdx, std::memory_order_release);
    return val;
  }

private:
  std::vector<T> data_{};
  alignas(kCacheLineSize) size_t capacity_{0};
  alignas(kCacheLineSize) std::atomic<size_t> readIdx_{0};
  alignas(kCacheLineSize) size_t writeIdxCached_{0};
  alignas(kCacheLineSize) std::atomic<size_t> writeIdx_{0};
  alignas(kCacheLineSize) size_t readIdxCached_{0};
};

}  // namespace common::containers