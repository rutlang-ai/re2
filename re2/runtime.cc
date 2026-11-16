// Minimal allocation and static-initialization ABI for freestanding RE2.
// No part of libstdc++, libc++ or libsupc++ is linked into the library.
#include <sched.h>
#include <stddef.h>
#include <stdlib.h>

#include <atomic>
#include <new>

void* operator new(size_t n) {
  void* p = malloc(n ? n : 1);
  if (!p) abort();
  return p;
}
void* operator new[](size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { free(p); }
void operator delete[](void* p) noexcept { free(p); }
void operator delete(void* p, size_t) noexcept { free(p); }
void operator delete[](void* p, size_t) noexcept { free(p); }
void* operator new(size_t n, const std::nothrow_t&) noexcept {
  return malloc(n ? n : 1);
}
void operator delete(void* p, const std::nothrow_t&) noexcept { free(p); }
// Guard storage is supplied by the compiler. The low byte must equal one only
// after initialization has completed. Bit 8 marks initialization in progress.
extern "C" int __cxa_guard_acquire(unsigned long long* p) {
  for (;;) {
    auto v = __atomic_load_n(p, __ATOMIC_ACQUIRE);
    if (v & 1) return 0;
    if (v == 0 && __atomic_compare_exchange_n(
                      p, &v, 256, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
      return 1;
    sched_yield();
  }
}
extern "C" void __cxa_guard_release(unsigned long long* p) {
  __atomic_store_n(p, 1, __ATOMIC_RELEASE);
}
extern "C" void __cxa_guard_abort(unsigned long long* p) {
  __atomic_store_n(p, 0, __ATOMIC_RELEASE);
}
extern "C" void __cxa_pure_virtual() { abort(); }

namespace std {
const nothrow_t nothrow{};
}
