// Copyright 2026 The RE2 Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.
#ifndef RE2_SUPPORT_H_
#define RE2_SUPPORT_H_

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atomic>
#include <limits>
#include <memory>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace re2 {
// This allocator never calls the C++ allocation/exception runtime.
// Like RE2's other allocation sites, an allocation failure is fatal.
template <class T>
struct Allocator {
  using value_type = T;
  using is_always_equal = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  Allocator() = default;
  template <class U>
  Allocator(const Allocator<U>&) {}
  T* allocate(size_t n) {
    if (n > SIZE_MAX / sizeof(T)) abort();
    void* p;
    if constexpr (alignof(T) > alignof(max_align_t)) {
      if (posix_memalign(&p, alignof(T), n * sizeof(T))) abort();
    } else {
      p = malloc(n ? n * sizeof(T) : 1);
    }
    if (!p) abort();
    return static_cast<T*>(p);
  }
  void deallocate(T* p, size_t) { free(p); }
  template <class U>
  bool operator==(const Allocator<U>&) const {
    return true;
  }
  template <class U>
  bool operator!=(const Allocator<U>&) const {
    return false;
  }
};
}  // namespace re2
#include "third_party/containa/container/btree_map.hpp"
#include "third_party/containa/container/btree_set.hpp"
#include "third_party/containa/container/dense_map.hpp"
#include "third_party/containa/container/devectra.hpp"
#include "third_party/containa/container/small_vectra.hpp"
#include "third_party/containa/container/static_vectra.hpp"

namespace re2 {
template <class T>
using Vector = stdb::container::small_vectra<T, 1, Allocator<T>>;
template <class K, class V, class C = std::less<>>
using Map =
    stdb::container::btree_map<K, V, C, Allocator<std::pair<const K, V>>>;
template <class K, class C = std::less<>>
using OrderedSet = stdb::container::btree_set<K, C, Allocator<K>>;

class StringView {
 public:
  using value_type = char;
  using const_iterator = const char*;
  static constexpr size_t npos = SIZE_MAX;
  constexpr StringView() : p_(nullptr), n_(0) {}
  constexpr StringView(const char* p, size_t n) : p_(p), n_(n) {}
  StringView(const char* p) : p_(p), n_(p ? strlen(p) : 0) {}
  template <class S, class = decltype(std::declval<const S&>().data()),
            class = decltype(std::declval<const S&>().size())>
  StringView(const S& s) : p_(s.data()), n_(s.size()) {}
  operator std::string_view() const { return std::string_view(p_, n_); }
  constexpr const char* data() const { return p_; }
  constexpr size_t size() const { return n_; }
  constexpr size_t length() const { return n_; }
  constexpr bool empty() const { return n_ == 0; }
  constexpr const char* begin() const { return p_; }
  constexpr const char* end() const { return n_ ? p_ + n_ : p_; }
  constexpr const char& operator[](size_t i) const { return p_[i]; }
  constexpr char front() const { return p_[0]; }
  constexpr char back() const { return p_[n_ - 1]; }
  void remove_prefix(size_t n) {
    if (n) p_ += n;
    n_ -= n;
  }
  void remove_suffix(size_t n) { n_ -= n; }
  StringView substr(size_t pos = 0, size_t n = npos) const {
    if (pos > n_) abort();
    return StringView(pos ? p_ + pos : p_, n < n_ - pos ? n : n_ - pos);
  }
  int compare(StringView b) const {
    size_t n = n_ < b.n_ ? n_ : b.n_;
    int c = n ? memcmp(p_, b.p_, n) : 0;
    return c ? c : (n_ > b.n_) - (n_ < b.n_);
  }
  size_t find(char c, size_t pos = 0) const {
    if (pos >= n_) return npos;
    auto p = static_cast<const char*>(memchr(p_ + pos, c, n_ - pos));
    return p ? static_cast<size_t>(p - p_) : npos;
  }
  size_t find(StringView s, size_t pos = 0) const {
    if (pos > n_ || s.n_ > n_ - pos) return npos;
    if (!s.n_) return pos;
    for (size_t i = pos; i <= n_ - s.n_; ++i)
      if (p_[i] == s[0] && memcmp(p_ + i, s.p_, s.n_) == 0) return i;
    return npos;
  }
  friend bool operator==(StringView a, StringView b) {
    return a.n_ == b.n_ && (!a.n_ || !memcmp(a.p_, b.p_, a.n_));
  }
  friend bool operator!=(StringView a, StringView b) { return !(a == b); }
  friend bool operator<(StringView a, StringView b) { return a.compare(b) < 0; }
  friend bool operator>(StringView a, StringView b) { return a.compare(b) > 0; }
  friend bool operator<=(StringView a, StringView b) {
    return a.compare(b) <= 0;
  }
  friend bool operator>=(StringView a, StringView b) {
    return a.compare(b) >= 0;
  }

 private:
  const char* p_;
  size_t n_;
};

}  // namespace re2
#include "third_party/smallstring/smallstring.hpp"
namespace re2 {
inline bool operator==(StringView a, const char* b) {
  return a == StringView(b);
}
inline bool operator==(const char* a, StringView b) {
  return StringView(a) == b;
}
inline bool operator!=(StringView a, const char* b) {
  return a != StringView(b);
}
inline bool operator!=(const char* a, StringView b) {
  return StringView(a) != b;
}
inline bool operator<(StringView a, const char* b) { return a < StringView(b); }
inline bool operator<(const char* a, StringView b) { return StringView(a) < b; }
inline bool operator>(StringView a, const char* b) { return a > StringView(b); }
inline bool operator>(const char* a, StringView b) { return StringView(a) > b; }
inline bool operator<=(StringView a, const char* b) {
  return a <= StringView(b);
}
inline bool operator<=(const char* a, StringView b) {
  return StringView(a) <= b;
}
inline bool operator>=(StringView a, const char* b) {
  return a >= StringView(b);
}
inline bool operator>=(const char* a, StringView b) {
  return StringView(a) >= b;
}
using String = small::small_string;
inline bool operator==(StringView a, const String& b) {
  return a == StringView(b);
}
inline bool operator==(const String& a, StringView b) {
  return StringView(a) == b;
}
inline bool operator!=(StringView a, const String& b) {
  return a != StringView(b);
}
inline bool operator!=(const String& a, StringView b) {
  return StringView(a) != b;
}
inline bool operator<(StringView a, const String& b) {
  return a < StringView(b);
}
inline bool operator<(const String& a, StringView b) {
  return StringView(a) < b;
}
inline bool operator>(StringView a, const String& b) {
  return a > StringView(b);
}
inline bool operator>(const String& a, StringView b) {
  return StringView(a) > b;
}
inline bool operator<=(StringView a, const String& b) {
  return a <= StringView(b);
}
inline bool operator<=(const String& a, StringView b) {
  return StringView(a) <= b;
}
inline bool operator>=(StringView a, const String& b) {
  return a >= StringView(b);
}
inline bool operator>=(const String& a, StringView b) {
  return StringView(a) >= b;
}

namespace internal {
template <class T>
using Deque = stdb::container::devectra<T, Allocator<T>>;
template <class>
class FunctionRef;
template <class R, class... A>
class FunctionRef<R(A...)> {
 public:
  FunctionRef() = default;
  FunctionRef(std::nullptr_t) {}
  template <class F>
  FunctionRef(const F& f)
      : p_(&f), call_([](const void* p, A... a) -> R {
          return (*static_cast<const F*>(p))(a...);
        }) {}
  explicit operator bool() const { return call_ != nullptr; }
  R operator()(A... a) const { return call_(p_, a...); }

 private:
  const void* p_ = nullptr;
  R (*call_)(const void*, A...) = nullptr;
};
using once_flag = std::atomic<unsigned>;
template <class F, class... A>
void call_once(once_flag& flag, F&& f, A&&... a) {
  if (flag.load(std::memory_order_acquire) == 2) return;
  unsigned expected = 0;
  if (flag.compare_exchange_strong(expected, 1, std::memory_order_acquire)) {
    std::forward<F>(f)(std::forward<A>(a)...);
    flag.store(2, std::memory_order_release);
  } else {
    while (flag.load(std::memory_order_acquire) != 2) sched_yield();
  }
}
class Mutex {
 public:
  Mutex() = default;
  ~Mutex() { pthread_rwlock_destroy(&m_); }
  void lock() {
    if (pthread_rwlock_wrlock(&m_)) abort();
  }
  void unlock() {
    if (pthread_rwlock_unlock(&m_)) abort();
  }
  void lock_shared() {
    if (pthread_rwlock_rdlock(&m_)) abort();
  }
  void unlock_shared() { unlock(); }

 private:
  pthread_rwlock_t m_ = PTHREAD_RWLOCK_INITIALIZER;
};
// Inline reader fast path; writers serialize through pthread and announce
// themselves before draining existing readers, so new readers cannot starve
// them.
class ReaderWriterMutex {
 public:
  ~ReaderWriterMutex() { pthread_mutex_destroy(&writers_); }
  void lock_shared() {
    for (;;) {
      uint32_t prior = state_.fetch_add(1, std::memory_order_acquire);
      if ((prior & ~kWriter) == kWriter - 1) abort();
      if (!(prior & kWriter)) return;
      state_.fetch_sub(1, std::memory_order_release);
      do { sched_yield(); }
      while (state_.load(std::memory_order_relaxed) & kWriter);
    }
  }
  void unlock_shared() { state_.fetch_sub(1, std::memory_order_release); }
  void lock() {
    if (pthread_mutex_lock(&writers_)) abort();
    state_.fetch_or(kWriter, std::memory_order_acquire);
    while (state_.load(std::memory_order_acquire) != kWriter) sched_yield();
  }
  void unlock() {
    // Failed readers can transiently increment the count while a writer owns
    // the lock. Preserve those increments until each failed reader backs out.
    state_.fetch_and(~kWriter, std::memory_order_release);
    if (pthread_mutex_unlock(&writers_)) abort();
  }

 private:
  static constexpr uint32_t kWriter = uint32_t{1} << 31;
  std::atomic<uint32_t> state_{0};
  pthread_mutex_t writers_ = PTHREAD_MUTEX_INITIALIZER;
};
class MutexLock {
 public:
  explicit MutexLock(Mutex& m) : m_(m) { m_.lock(); }
  ~MutexLock() { m_.unlock(); }

 private:
  Mutex& m_;
};
template <class T, size_t N = 1>
using InlinedVector = stdb::container::small_vectra<T, N, Allocator<T>>;
// Containa owns inline scratch objects; larger arrays need one allocation.
template <class T, size_t N = 1>
class FixedArray {
 public:
  explicit FixedArray(size_t n) : inline_(n <= N ? n : 0), n_(n) {
    if (n > N) {
      heap_ = Allocator<T>().allocate(n);
      std::uninitialized_value_construct_n(heap_, n);
    }
  }
  FixedArray(size_t n, const T& value) : inline_(n <= N ? n : 0, value), n_(n) {
    if (n > N) {
      heap_ = Allocator<T>().allocate(n);
      std::uninitialized_fill_n(heap_, n, value);
    }
  }
  ~FixedArray() {
    if (heap_) {
      std::destroy_n(heap_, n_);
      Allocator<T>().deallocate(heap_, n_);
    }
  }
  FixedArray(const FixedArray&) = delete;
  FixedArray& operator=(const FixedArray&) = delete;
  T* data() { return heap_ ? heap_ : inline_.data(); }
  size_t size() const { return n_; }

 private:
  stdb::container::static_vectra<T, N> inline_;
  T* heap_ = nullptr;
  size_t n_;
};
template <class T>
class Span {
 public:
  Span(T* p, size_t n) : p_(p), n_(n) {}
  T* begin() const { return p_; }
  T* end() const { return n_ ? p_ + n_ : p_; }
  size_t size() const { return n_; }
  friend bool operator==(Span a, Span b) {
    return a.n_ == b.n_ && std::equal(a.begin(), a.end(), b.begin());
  }

 private:
  T* p_;
  size_t n_;
};
struct HashState {
  size_t h = 0x9e3779b97f4a7c15ULL;
  template <class T>
  void add(const T& x) {
    if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
      mix(static_cast<size_t>(x));
    } else if constexpr (std::is_pointer_v<T>) {
      mix(reinterpret_cast<size_t>(x));
    } else if constexpr (std::is_convertible_v<T, StringView>) {
      StringView view(x);
      mix(stdb::container::dense_hash<std::string_view>{}(
          std::string_view(view.data(), view.size())));
    } else {
      *this = RE2HashValue(*this, x);
    }
  }
  template <class T>
  void add(Span<T> x) {
    static_assert(std::is_integral_v<T> ||
                  std::is_integral_v<std::remove_const_t<T>>);
    mix(stdb::container::dense_hash<std::string_view>{}(std::string_view(
        reinterpret_cast<const char*>(x.begin()), x.size() * sizeof(T))));
  }
  void mix(size_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    h ^= x + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
  }
  template <class... T>
  static HashState combine(HashState h, const T&... x) {
    (h.add(x), ...);
    return h;
  }
};
template <class T>
struct Hash {
  size_t operator()(const T& x) const {
    HashState h;
    h.add(x);
    return h.h;
  }
};
template <class K, class V, class H = Hash<K>, class E = std::equal_to<K>>
using flat_hash_map =
    stdb::container::dense_map<K, V, H, E, Allocator<std::pair<K, V>>>;
template <class K, class H = Hash<K>, class E = std::equal_to<K>>
using flat_hash_set = stdb::container::dense_map<K, void, H, E, Allocator<K>>;
inline bool ascii_isdigit(unsigned char c) { return c >= '0' && c <= '9'; }
inline bool ascii_isalnum(unsigned char c) {
  return ascii_isdigit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
inline bool ascii_isspace(unsigned char c) {
  return c == ' ' || (c >= '\t' && c <= '\r');
}
inline const char* PrintArg(const String& s) { return s.c_str(); }
inline String PrintArg(StringView s) { return String(s); }
template <class T>
inline T PrintArg(T v) {
  return v;
}
// All format strings in the engine are compile-time literals. String arguments
// are converted explicitly before entering the C variadic formatting API.
inline const char* CArg(const String& s) { return s.c_str(); }
template <class T>
inline T CArg(T v) {
  return v;
}
template <class... T>
String StrFormat(const char* f, const T&... v) {
  if constexpr (sizeof...(v) == 0)
    return String(f);
  else {
    auto args = std::tuple{PrintArg(v)...};
    return std::apply(
        [&](const auto&... a) {
          int n = snprintf(nullptr, 0, f, CArg(a)...);
          if (n < 0) abort();
          String s;
          s.resize(n);
          snprintf(s.data(), n + 1, f, CArg(a)...);
          return s;
        },
        args);
  }
}
template <class... T>
int FPrintF(FILE* fp, const char* f, const T&... v) {
  auto s = StrFormat(f, v...);
  return static_cast<int>(fwrite(s.data(), 1, s.size(), fp));
}
template <class... T>
int PrintF(const char* f, const T&... v) {
  return FPrintF(stdout, f, v...);
}
enum Severity {
  INFO,
  WARNING,
  ERROR,
  FATAL,
  QFATAL = FATAL,
#ifdef NDEBUG
  DFATAL = ERROR
#else
  DFATAL = FATAL
#endif
};
class Log {
 public:
  Log(Severity s, const char* file, int line) : s_(s) {
    fprintf(stderr, "%s:%d: ", file, line);
  }
  ~Log() {
    fputc('\n', stderr);
    if (s_ == FATAL) abort();
  }
  Log& operator<<(StringView s) {
    if (s.size()) fwrite(s.data(), 1, s.size(), stderr);
    return *this;
  }
  Log& operator<<(const String& s) { return *this << StringView(s); }
  Log& operator<<(const char* s) { return *this << StringView(s); }
  Log& operator<<(char c) {
    fputc(c, stderr);
    return *this;
  }
  template <class T>
  Log& operator<<(T x) {
    if constexpr (std::is_pointer_v<T>)
      fprintf(stderr, "%p", static_cast<const void*>(x));
    else if constexpr (std::is_floating_point_v<T>)
      fprintf(stderr, "%g", static_cast<double>(x));
    else if constexpr (std::is_signed_v<T>)
      fprintf(stderr, "%lld", static_cast<long long>(x));
    else
      fprintf(stderr, "%llu", static_cast<unsigned long long>(x));
    return *this;
  }

 private:
  Severity s_;
};
// Makes checks a single expression while allowing streamed diagnostic text.
struct LogVoidify {
  void operator&(Log&) const {}
};
}  // namespace internal
}  // namespace re2
#define RE2_ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#define RE2_NO_THREAD_SAFETY_ANALYSIS
#define RE2_LOG(s) ::re2::internal::Log(::re2::internal::s, __FILE__, __LINE__)
#define RE2_CHECK(c)                                   \
  (c) ? (void)0                                        \
      : ::re2::internal::LogVoidify() & RE2_LOG(FATAL) \
                                            << "Check failed: " #c " "
#define RE2_CHECK_EQ(a, b) RE2_CHECK((a) == (b))
#define RE2_CHECK_NE(a, b) RE2_CHECK((a) != (b))
#define RE2_CHECK_LT(a, b) RE2_CHECK((a) < (b))
#define RE2_CHECK_LE(a, b) RE2_CHECK((a) <= (b))
#define RE2_CHECK_GT(a, b) RE2_CHECK((a) > (b))
#define RE2_CHECK_GE(a, b) RE2_CHECK((a) >= (b))
#ifdef NDEBUG
#define RE2_DCHECK(c) RE2_CHECK(true || (c))
#else
#define RE2_DCHECK(c) RE2_CHECK(c)
#endif
#define RE2_DCHECK_EQ(a, b) RE2_DCHECK((a) == (b))
#define RE2_DCHECK_NE(a, b) RE2_DCHECK((a) != (b))
#define RE2_DCHECK_LT(a, b) RE2_DCHECK((a) < (b))
#define RE2_DCHECK_LE(a, b) RE2_DCHECK((a) <= (b))
#define RE2_DCHECK_GT(a, b) RE2_DCHECK((a) > (b))
#define RE2_DCHECK_GE(a, b) RE2_DCHECK((a) >= (b))
#endif
