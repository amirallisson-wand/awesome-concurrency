# Synchronization Primitives

This directory contains implementations of various synchronization primitives for concurrent programming.

## MCS Spinlock

**Motivated by**

[Linux Kernel MCS Spinlock implementation](`https://github.com/torvalds/linux/blob/master/kernel/locking/mcs_spinlock.h`)

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

## Future Implementations

Planned synchronization primitives:
- **CLH Spinlock** - Similar to MCS but using implicit queue
- **Ticket Lock** - Simple fair spinlock with bounded space
- **Reader-Writer Locks** - Allow multiple readers or single writer
- **Seqlock** - Optimistic read lock for data structures

