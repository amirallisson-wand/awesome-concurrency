# Synchronization Primitives

This directory contains implementations of various synchronization primitives for concurrent programming.

## MCS Spinlock

**Motivated by**

[Linux Kernel MCS Spinlock implementation](https://github.com/torvalds/linux/blob/master/kernel/locking/mcs_spinlock.h)

**File:** [`mcs_spinlock.hpp`](mcs_spinlock.hpp)

### Overview

The MCS (Mellor-Crummey and Scott) spinlock is a scalable queue-based spinlock that provides FIFO ordering and excellent cache performance under high contention.

### Key Features

* **FIFO Fairness** — Threads acquire the lock strictly in the order they arrived, ensuring fairness and preventing starvation.
* **Cache-Friendly Spinning** — Each thread spins on a *private, local* flag in its own queue node, drastically reducing cache coherence traffic and eliminating false sharing.
* **No Thundering Herd** — Upon unlock, only the *next* waiting thread is notified, avoiding unnecessary wakeups and contention storms.
* **Excellent Scalability** — Scales efficiently under high contention, with performance that remains stable even as thread count increases.
* **RAII-Based Guard** — Lock acquisition and release are managed safely through a scope-based RAII wrapper, preventing accidental unlock omissions and simplifying usage.
* **Deterministic Behavior** — The lock queue ensures predictable acquisition order and timing characteristics, ideal for low-level concurrent systems.

### How It Works

MCS uses a linked queue of waiting threads:

1. **Enqueue Phase**: Thread atomically adds itself to the tail of the queue
2. **Spin Phase**: Thread spins on its own local `is_owner` flag
3. **Dequeue Phase**: Lock holder passes ownership to the next thread in the queue

This approach ensures each thread only spins on its own cache line, avoiding the cache coherence traffic that plagues naive spinlocks.

### Usage

```cpp
#include "thread/sync/mcs_spinlock.hpp"

thread::sync::QueueSpinLock spinlock;
int shared_data = 0;

{
    thread::sync::QueueSpinLock::Guard guard{spinlock};
    ++shared_data;
}
```


### When to Use

**Good for:**
- Short critical sections
- High thread contention scenarios
- When fairness (FIFO ordering) is important

**Avoid when:**
- Critical sections are long (use a mutex instead)
- Contention is very low (simpler spinlocks may be faster)

### Example

See [`examples/sync/mcs_example.cpp`](../../../examples/sync/mcs_example.cpp) for a complete working example with benchmarking.

### References

- Original Paper: ["Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors"](https://www.cs.rochester.edu/~scott/papers/1991_TOCS_synch.pdf) by John M. Mellor-Crummey and Michael L. Scott (1991)

---

## Ticket Lock

**File:** [`ticket_lock.hpp`](ticket_lock.hpp)

### Overview

Ticket Lock is a simple and fair spinlock that uses a ticket-based algorithm. Threads take a ticket and wait until their number is called, ensuring strict FIFO ordering.

### Key Features

* **FIFO Fairness** — Threads acquire the lock in the exact order they requested it, preventing starvation
* **Simple Implementation** — Uses only two atomic counters, making it easy to understand and verify
* **Bounded Space** — O(1) space per lock (no per-thread nodes needed)
* **BasicLockable Concept** — Implements standard `lock()`, `try_lock()`, and `unlock()` interface
* **Predictable Behavior** — Deterministic lock acquisition order

### How It Works

The lock maintains two atomic counters:
1. **`next_free_ticket_`** — Counter that dispenses tickets to arriving threads
2. **`owner_ticket_`** — Current ticket number that can acquire the lock

Algorithm:
1. Thread atomically increments `next_free_ticket_` and receives a ticket number
2. Thread spins until `owner_ticket_` equals its ticket number
3. On unlock, `owner_ticket_` is incremented, allowing the next thread to proceed

### Usage

```cpp
#include "thread/sync/ticket_lock.hpp"

thread::sync::TicketLock lock;
int shared_data = 0;

// With std::lock_guard
{
    std::lock_guard<thread::sync::TicketLock> guard(lock);
    ++shared_data;
}

// Manual lock/unlock
lock.lock();
++shared_data;
lock.unlock();

// Try lock
if (lock.try_lock()) {
    ++shared_data;
    lock.unlock();
}
```

### Performance Characteristics

* **Low Contention**: Good performance, similar to simple spinlocks
* **High Contention**: Better than naive spinlocks but worse than MCS (all threads spin on same cache line)
* **Fairness**: Strict FIFO ordering prevents starvation
* **Memory**: O(1) per lock (2 atomic counters)

### When to Use

**Good for:**
- When strict FIFO fairness is required
- Simple use cases where code clarity is important
- Moderate contention scenarios
- When per-thread storage overhead is a concern

**Avoid when:**
- Very high contention (MCS scales better)
- Lock is held for long periods (use mutex instead)
- Priority inversion is a concern

### Tests

See [`tests/sync/ticket_lock_test.cpp`](../../../tests/sync/ticket_lock_test.cpp) for comprehensive test suite.

---

## TTAS Spinlock (Test-and-Test-and-Set)

**File:** [`ttas_spinlock.hpp`](ttas_spinlock.hpp)

### Overview

TTAS (Test-and-Test-and-Set) is an improved spinlock that reduces cache coherence traffic by performing a read-only check before attempting an atomic compare-exchange. This optimization significantly improves performance under contention compared to naive Test-and-Set (TAS) spinlocks.

### Key Features

* **Reduced Cache Traffic** — Read-only spinning reduces cache coherence messages
* **Fast Uncontended Path** — Single compare-exchange when lock is available
* **Simple and Efficient** — Minimal overhead with good performance characteristics
* **BasicLockable Concept** — Standard lock interface compatible with `std::lock_guard`
* **No Fairness Guarantees** — Lock acquisition order is non-deterministic

### How It Works

The algorithm uses two-phase acquisition:

1. **Test Phase**: Read the lock state with `memory_order_relaxed` (no cache line invalidation)
2. **Test-and-Set Phase**: When lock appears free, attempt atomic compare-exchange

This approach keeps the cache line in shared state during spinning, only generating invalidation traffic when the lock becomes available.

### Usage

```cpp
#include "thread/sync/ttas_spinlock.hpp"

thread::sync::TASSpinLock lock;
int shared_data = 0;

// With std::lock_guard
{
    std::lock_guard<thread::sync::TASSpinLock> guard(lock);
    ++shared_data;
}

// Manual lock/unlock
lock.lock();
++shared_data;
lock.unlock();

// Try lock
if (lock.try_lock()) {
    ++shared_data;
    lock.unlock();
}
```

### Performance Characteristics

* **Low Contention**: Excellent performance, similar to native atomic operations
* **Medium Contention**: Good performance due to reduced cache traffic
* **High Contention**: Degrades under extreme contention (thundering herd on unlock)
* **Fairness**: No fairness guarantees, potential for starvation
* **Memory**: O(1) per lock (single atomic bool)

### When to Use

**Good for:**
- Low to medium contention scenarios
- Very short critical sections
- When lock overhead must be minimal
- When fairness is not required

**Avoid when:**
- Fairness is important (use Ticket or MCS lock)
- High contention expected (use MCS lock)
- Risk of starvation is unacceptable
- Critical sections are long (use mutex)

### Tests

See [`tests/sync/ttas_spinlock_test.cpp`](../../../tests/sync/ttas_spinlock_test.cpp) for comprehensive test suite.

---

## Three-State Mutex

**File:** [`mutex.hpp`](mutex.hpp)

### Overview

The Three-State Mutex is an efficient mutex implementation that combines fast userspace operations with kernel-level blocking. It uses a three-state design to optimize for the common case where locks are uncontended, avoiding expensive system calls on both lock and unlock operations.

### Key Features

* **Fast Path (No Syscalls)** — Lock and unlock operations complete in userspace without kernel involvement when uncontended
* **Three-State Design** — Tracks whether the lock has waiting threads to minimize futex operations
* **Futex-Based Blocking** — Uses Linux futex for efficient kernel-level blocking instead of busy-waiting

### The Three States

The mutex uses three distinct states to optimize performance:

1. **`Unlocked` (0)** — Mutex is free and available for acquisition
2. **`LockedNoWaiters` (1)** — Mutex is held, but no other threads are waiting
3. **`LockedHasWaiters` (2)** — Mutex is held, and at least one thread is blocked waiting

This state tracking allows the unlock operation to know whether it needs to wake waiting threads.

**Slow Path (Contention):**
1. Thread transitions state from `LockedNoWaiters` → `LockedHasWaiters`
2. Thread calls `futex_wait()` to block in kernel until lock is released
3. On unlock, holder sets state to `Unlocked` and calls `futex_wake()` to wake waiters
4. Woken threads compete to acquire the now-free lock

### Performance Characteristics

* **Uncontended**: Extremely fast, single atomic CAS operation (comparable to spinlock)
* **Low Contention**: Good performance, threads block briefly then acquire lock
* **High Contention**: Excellent performance, threads sleep rather than spinning
* **Long Critical Sections**: Ideal use case, blocked threads don't waste CPU
* **Fairness**: No strict fairness, kernel scheduler determines wakeup order

## Comparison

| Feature | Three-State Mutex | MCS | Ticket Lock | TTAS |
|---------|-------------------|-----|-------------|------|
| Fairness | ⚠️ Kernel scheduler | ✅ FIFO | ✅ FIFO | ❌ None |
| Scalability | ✅ Excellent | ✅ Excellent | ⚠️ Moderate | ⚠️ Moderate |
| Memory/Lock | O(1) | O(1) + O(1)/thread | O(1) | O(1) |
| Complexity | Medium | High | Low | Low |
| Blocking | ✅ Kernel sleep | ❌ Busy-wait | ❌ Busy-wait | ❌ Busy-wait |
| CPU Efficiency | ✅ No waste | ⚠️ Spins | ⚠️ Spins | ⚠️ Spins |
| Cache Traffic | High | Minimal | High | Medium |

---

## Future Implementations

Planned synchronization primitives:
- **CLH Spinlock** - Similar to MCS but using implicit queue
- **Reader-Writer Locks** - Allow multiple readers or single writer
- **Seqlock** - Optimistic read lock for data structures
- **Condition Variable** - Wait/notify coordination with mutex
- **Semaphore** - Counting synchronization primitive
