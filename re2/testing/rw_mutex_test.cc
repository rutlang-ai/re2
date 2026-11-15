// Exercise the reader/writer synchronization without a C++ runtime.
#include "re2/support.h"
re2::internal::ReaderWriterMutex mu;
int value = 0, mirror = 0;
void* worker(void* writer) {
  for (int i = 0; i < 5000; ++i) {
    if (writer) {
      mu.lock();
      ++value;
      sched_yield();
      mirror = value;
      mu.unlock();
    } else {
      mu.lock_shared();
      int v = value;
      sched_yield();
      if (mirror != v) abort();
      mu.unlock_shared();
    }
  }
  return nullptr;
}
int main() {
  pthread_t ts[8];
  for (int i = 0; i < 8; ++i)
    if (pthread_create(&ts[i], nullptr, worker, reinterpret_cast<void*>(i < 2)))
      abort();
  for (auto t : ts) pthread_join(t, nullptr);
  if (value != 10000) abort();
  puts("rw mutex stress passed");
}
