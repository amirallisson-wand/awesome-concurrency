# Lock-Free SPSC Ring Buffers

High-performance, lock-free ring buffers designed for Single Producer Single Consumer scenarios.

**Motivated by:** ["Optimizing a ring buffer for throughput" by Erik Rigtorp](https://rigtorp.se/ringbuffer/)

## Overview

This module provides two implementations of lock-free circular buffers optimized for SPSC workloads:

- **RingBuffer** - Standard SPSC ring buffer with straightforward atomic operations
- **FastRingBuffer** - Cache-optimized variant with local index caching to reduce atomic loads

Both implementations provide:
- Lock-free operation using atomic compare-and-swap
- Zero-copy semantics with move operations
- Support for not default constructible types
- Bounded capacity with dynamic lazy initialization
- Cache-line padding to prevent false sharing
- Efficient use of memory orders

## RingBuffer

The standard SPSC ring buffer implementation with direct atomic operations.

### Memory Ordering

The implementation uses careful memory ordering:

- **Relaxed loads** for indices owned by the current thread
- **Acquire loads** when reading the other thread's index
- **Release stores** when updating indices to make data visible

This ensures:
1. Data written before `writeIdx_` update is visible when consumer reads
2. Data read before `readIdx_` update prevents producer from overwriting
3. Minimal synchronization overhead on modern CPUs

## FastRingBuffer

An optimized variant that caches remote thread indices to reduce atomic operations.

### Key Optimization

The standard ring buffer performs an atomic load of the remote thread's index on every operation. FastRingBuffer caches this index and only refreshes when necessary.

### Additional State

```cpp
size_t writeIdxCached_;  // Producer's cached copy of consumer's read index
size_t readIdxCached_;   // Consumer's cached copy of producer's write index
```

Both cached indices are aligned to separate cache lines.

## Limitations

**Single Producer Single Consumer Only**

These implementations are NOT thread-safe for multiple producers or consumers. The lock-free design relies on:
- Only one thread modifying `writeIdx_`
- Only one thread modifying `readIdx_`

Violating this will cause data races and undefined behavior.
