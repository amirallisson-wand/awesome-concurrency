#pragma once

#include <atomic>
#include <thread/util/spin_wait.hpp>

namespace thread::sync {

class QueueSpinLock {
public:
  class Guard {
    friend class QueueSpinLock;

  public:
    explicit Guard(QueueSpinLock& host) : host_(host) { host_.Acquire(this); }

    // Non-copyable
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;

    // Non-movable
    Guard(Guard&&) = delete;
    Guard& operator=(Guard&&) = delete;

    ~Guard() { host_.Release(this); }

  private:
    QueueSpinLock& host_;
    std::atomic<Guard*> next_{nullptr};
    std::atomic<bool> is_owner_{false};
  };

private:
  void Enqueue(Guard* waiter) {
    // Phase 1: Acquire the tail
    // synchronize with prior releases and future acquires of tail_
    auto prev_tail = tail_.exchange(waiter, std::memory_order_acq_rel);
    if (prev_tail == nullptr) {
      waiter->is_owner_.store(true, std::memory_order_release);
      return;
    }

    // Phase 2: Link the previous tail to the new waiter
    prev_tail->next_.store(waiter, std::memory_order_release);
  }

  void Dequeue(Guard* waiter) {
    auto old_waiter = waiter;
    // synchronize with prior releases and future acquires of tail_
    if (tail_.compare_exchange_strong(
          /* expected */ old_waiter,
          /* desired */ nullptr,
          /* success */ std::memory_order_acq_rel,
          /* failure */ std::memory_order_relaxed)) {
      return;
    }

    // At this point we are sure that there are other waiters enqueued after us.
    // It is possible that the next waiter has completed Phase 1
    // but not Phase 2 yet, so we need to spin until it is done.
    SpinUntilNextWaiter(waiter);
    waiter->next_.load()->is_owner_.store(true, std::memory_order_release);
  }

  void SpinUntilNextWaiter(Guard* waiter) {
    while (waiter->next_.load(std::memory_order_acquire) == nullptr) {
      thread::util::SpinLoopHint();
    }
  }

  void Acquire(Guard* waiter) {
    Enqueue(waiter);
    while (!waiter->is_owner_.load(std::memory_order_acquire)) {
      thread::util::SpinLoopHint();
    }
  }

  void Release(Guard* owner) { Dequeue(owner); }

private:
  std::atomic<Guard*> tail_{nullptr};
};

}  // namespace thread::sync
