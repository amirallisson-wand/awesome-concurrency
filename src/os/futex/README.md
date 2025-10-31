# Futex (Fast Userspace Mutex)

**Motivated by:** [futex_like](https://gitlab.com/Lipovsky/futex_like) by Roman Lipovsky

## Overview

The Linux futex (fast userspace mutex) is a fundamental building block for efficient synchronization primitives. It enables user-space threads to implement locks that avoid expensive system calls in the common (uncontended) case, only transitioning to the kernel when threads need to block.

This module provides a thin, type-safe C++ wrapper around the Linux futex system call.

### Kernel Implementation

The futex subsystem lives in [`kernel/futex/waitwake.c`](https://github.com/torvalds/linux/blob/master/kernel/futex/waitwake.c). Key design aspects:

**Hash Table Queuing**
- Futex addresses are hashed to buckets containing wait queues
- Multiple futexes may share a bucket, but collisions only affect performance
- Per-bucket spinlocks protect queue operations

**Wait Queue Management**
- Waiting threads are enqueued with proper priority handling
- Memory barriers ensure visibility of updates before blocking
- Lost wake-ups are prevented through careful ordering

**Wake Optimization**
- Before acquiring the bucket lock, kernel checks if waiters exist
- Empty queues allow immediate return from `futex_wake()`
- This fast-path check avoids lock contention on uncontended futexes

## References

**Documentation:**
- [Futex manual page](https://man7.org/linux/man-pages/man2/futex.2.html) - Official Linux man page
- ["Futexes Are Tricky" by Ulrich Drepper](https://www.akkadia.org/drepper/futex.pdf) - Essential reading on futex usage and pitfalls

**Source Code:**
- [Linux kernel futex implementation](https://github.com/torvalds/linux/blob/master/kernel/futex/waitwake.c)
