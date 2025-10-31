#pragma once

#include <atomic>
#include <cstdint>
#include <os/futex/futex.hpp>

namespace thread::sync {

// Satisfies the BasicLockable concept https://en.cppreference.com/w/cpp/named_req/BasicLockable
class Mutex {
  struct State {
    enum _ : uint32_t {
      Unlocked = 0,
      LockedNoWaiters = 1,
      LockedHasWaiters = 2,
    };
  };

public:
  void lock() {
    if (FastPathLock()) {
      return;
    }

    while (!SlowPathLock()) {
    }
  }

  void unlock() {
    if (UnlockFastPath()) {
      return;
    }
    lock_.store(State::Unlocked, std::memory_order_release);
    os::futex::WakeAll(reinterpret_cast<uint32_t*>(&lock_));
  }

private:
  bool CompareExchange(uint32_t expected, uint32_t desired, std::memory_order success) {
    return lock_.compare_exchange_strong(expected, desired, success,
                                         /* failure */ std::memory_order_relaxed);
  }

  bool FastPathLock() {
    return CompareExchange(
      /* expected */ State::Unlocked,
      /* desired */ State::LockedNoWaiters,
      /* success */ std::memory_order_acquire);
  }

  bool SlowPathLock() {
    CompareExchange(
      /* expected */ State::LockedNoWaiters,
      /* desired */ State::LockedHasWaiters,
      /* success */ std::memory_order_acquire);

    os::futex::Wait(reinterpret_cast<uint32_t*>(&lock_), /* oldval */ State::LockedHasWaiters);
    if (CompareExchange(
          /* expected */ State::Unlocked,
          /* desired */ State::LockedHasWaiters,
          /* success */ std::memory_order_acquire)) {
      return true;
    }
    return false;
  }

  bool UnlockFastPath() {
    return CompareExchange(
      /* expected */ State::LockedNoWaiters,
      /* desired */ State::Unlocked,
      /* success */ std::memory_order_release);
  }

  std::atomic<uint32_t> lock_{State::Unlocked};
};

}  // namespace thread::sync
