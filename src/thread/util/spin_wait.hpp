#pragma once

namespace thread::util {

// Platform-specific spin loop hint for busy-waiting
// Tells the CPU we're in a spin loop to improve power consumption
// and hyper-threading performance
inline void SpinLoopHint() {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
  asm volatile("pause\n" : : : "memory");
#elif defined(__aarch64__) || defined(_M_ARM64)
  asm volatile("yield\n" : : : "memory");
#else
  // No specific instruction for other architectures
  ;
#endif
}

}  // namespace thread::util
