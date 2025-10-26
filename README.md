# Awesome Concurrency

A C++ concurrency learning repository with implementations of various synchronization primitives and concurrent data structures.

## Current Implementations

### MCS Spinlock (Mellor-Crummey and Scott)

A scalable queue-based spinlock that provides:
- **FIFO ordering**: Threads acquire the lock in the order they request it
- **Cache efficiency**: Each thread spins on its own node, reducing cache contention
- **Scalability**: Performance doesn't degrade with increased contention

Located in `src/thread/sync/mcs_spinlock.hpp`

### Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Run tests
ctest --verbose

# Run example
./examples/mcs_example
```

### Build Options

- `BUILD_TESTS=ON/OFF` - Enable/disable building tests (default: ON)
- `BUILD_EXAMPLES=ON/OFF` - Enable/disable building examples (default: ON)

Example:
```bash
cmake -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=ON ..
```

## License

This is an educational project. Feel free to use and modify as needed for learning purposes.

