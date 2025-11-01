#pragma once

#include <atomic>
#include <thread/util/spin_wait.hpp>

namespace thread::sync {

// Satisfies the BasicLockable concept https://en.cppreference.com/w/cpp/named_req/BasicLockable
class TASSpinLock {
public:
  void lock() {
    while (!TryCompareExchange()) {
      // relaxed load is sufficient because synchronization with other threads is established by the
      // outer loop
      while (locked_.load(std::memory_order::relaxed)) {
        thread::util::SpinLoopHint();
      }
    }
  }

  bool try_lock() {
    return TryCompareExchange();
  }

  void unlock() {
    locked_.store(false, std::memory_order::release);
  }

private:
  bool TryCompareExchange() {
    bool expected = false;
    return locked_.compare_exchange_weak(expected,
                                         /* desired */ true,
                                         /* success */ std::memory_order::acquire,
                                         /* failure */ std::memory_order::relaxed);
  }

  std::atomic<bool> locked_{false};
};

}  // namespace thread::sync