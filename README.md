# Awesome Concurrency

A C++ concurrency learning repository with implementations of various synchronization primitives and concurrent data structures.

## Project Structure

```
awesome-concurrency/
├── src/
│   └── thread/
│       ├── sync/          # Synchronization primitives (spinlocks, mutexes, etc.)
│       └── util/          # Utility functions (spin wait hints, etc.)
├── examples/
│   └── sync/              # Examples demonstrating synchronization primitives
├── tests/
│   └── sync/              # Google Test-based unit tests
└── .vscode/               # VS Code tasks and shortcuts
```

## Requirements

- CMake 3.14 or higher
- C++23 compatible compiler (GCC 12+, Clang 15+)
- pthread support
- clang-format (optional, for code formatting)

## Building

```bash
# Configure
cmake -B build -S .

# Build
cmake --build build

# Run tests
cmake --build build --target run-tests

# Format code
cmake --build build --target format
```

## Current Implementations

### Synchronization Primitives

See [src/thread/sync/](src/thread/sync/) for detailed documentation.

- **MCS Spinlock** - Scalable queue-based spinlock with FIFO ordering

### Utilities

- **[SpinLoopHint](src/thread/util/spin_wait.hpp)** - Platform-specific CPU hints for busy-waiting

## Running Tests

```bash
# Run all tests with CTest
cd build && ctest --output-on-failure

# Run specific test suite
./build/tests/sync/mcs_test

# Run with gtest filters
./build/tests/sync/mcs_test --gtest_filter=MCSSpinlockTest.MutualExclusion
```

## Running Examples

```bash
# Run MCS spinlock example
./build/examples/sync/mcs_example
```

## License

This is an educational project. Feel free to use and modify as needed for learning purposes.
