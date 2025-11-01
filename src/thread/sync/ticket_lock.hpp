#pragma once

#include <atomic>
#include <cstdint>
#include <os/constants.hpp>
#include <thread/util/spin_wait.hpp>

namespace thread::sync {

// Satisfies the BasicLockable concept https://en.cppreference.com/w/cpp/named_req/BasicLockable
class TicketLock {
  // use uint64_t to avoid ABA due to overflow
  // however, atomics for uint64_t are not always available
  // and if they are, they may be slower than 32-bit atomics
  using Ticket = uint64_t;

public:
  void lock() {
    const Ticket this_thread_ticket = next_free_ticket_.fetch_add(1, std::memory_order_relaxed);

    while (this_thread_ticket != owner_ticket_.load(std::memory_order_acquire)) {
      thread::util::SpinLoopHint();
    }
  }

  bool try_lock() {
    Ticket current_owner_ticket = owner_ticket_.load(std::memory_order_relaxed);
    return next_free_ticket_.compare_exchange_strong(/* expected */ current_owner_ticket,
                                                     /* desired */ current_owner_ticket + 1,
                                                     /* success */ std::memory_order_acquire,
                                                     /* failure */ std::memory_order_relaxed);
  }

  void unlock() {
    owner_ticket_.fetch_add(1, std::memory_order_release);
  }

private:
  alignas(os::kL1CacheLineSize) std::atomic<Ticket> next_free_ticket_{0};
  alignas(os::kL1CacheLineSize) std::atomic<Ticket> owner_ticket_{0};
};

}  // namespace thread::sync
