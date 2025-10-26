#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "thread/sync/mcs_spinlock.hpp"

class SharedCounter {
public:
  explicit SharedCounter(int initial_value = 0) : value_(initial_value) {}

  void Increment() {
    thread::sync::QueueSpinLock::Guard guard{lock_};
    ++value_;
  }

  int GetValue() const { return value_; }

private:
  thread::sync::QueueSpinLock lock_;
  int value_;
};

struct BenchmarkConfig {
  int num_threads = 4;
  int increments_per_thread = 100000;
};

class BenchmarkRunner {
public:
  explicit BenchmarkRunner(const BenchmarkConfig& config) : config_(config) {}

  void Run() {
    PrintHeader();

    SharedCounter counter;
    auto start = std::chrono::high_resolution_clock::now();

    RunWorkers(counter);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    PrintResults(counter.GetValue(), duration.count());
  }

private:
  void PrintHeader() const {
    std::cout << "Starting MCS Spinlock benchmark...\n";
    std::cout << "Threads: " << config_.num_threads << "\n";
    std::cout << "Increments per thread: " << config_.increments_per_thread << "\n\n";
  }

  void RunWorkers(SharedCounter& counter) {
    std::vector<std::thread> threads;

    for (int i = 0; i < config_.num_threads; ++i) {
      threads.emplace_back([&counter, i, this]() {
        for (int j = 0; j < config_.increments_per_thread; ++j) {
          counter.Increment();
        }
        std::cout << "Thread " << i << " completed\n";
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }
  }

  void PrintResults(int actual, long long duration_ms) const {
    int expected = config_.num_threads * config_.increments_per_thread;

    std::cout << "\n=== Results ===\n";
    std::cout << "Time taken: " << duration_ms << " ms\n";
    std::cout << "Final counter value: " << actual << "\n";
    std::cout << "Expected value: " << expected << "\n";

    if (actual == expected) {
      std::cout << "MCS Spinlock works correctly!\n";
    } else {
      std::cout << "MCS Spinlock failed!\n";
      throw std::runtime_error("MCS Spinlock failed");
    }
  }

  BenchmarkConfig config_;
};

int main() {
  BenchmarkConfig config{
    .num_threads = 4,
    .increments_per_thread = 100000,
  };

  BenchmarkRunner runner(config);
  runner.Run();
}
