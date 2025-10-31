#include <linux/futex.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

namespace os::futex {

inline void SetTimeout(struct timespec& timeout, uint32_t micros) {
  timeout.tv_sec = micros / 1'000'000;
  timeout.tv_nsec = (micros % 1'000'000) * 1'000;
}

inline int WaitTimed(uint32_t* loc, uint32_t old, uint32_t micros) {
  struct timespec timeout;
  SetTimeout(timeout, micros);

  return syscall(SYS_futex, loc, FUTEX_WAIT_PRIVATE, old, &timeout, nullptr, 0);
}

inline int Wait(uint32_t* loc, uint32_t old) {
  return syscall(SYS_futex, loc, FUTEX_WAIT_PRIVATE, old, nullptr, nullptr, 0);
}

inline int WakeOne(uint32_t* loc) {
  return syscall(SYS_futex, loc, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
}

inline int WakeAll(uint32_t* loc) {
  return syscall(SYS_futex, loc, FUTEX_WAKE_PRIVATE, INT32_MAX, nullptr, nullptr, 0);
}

}  // namespace os::futex
