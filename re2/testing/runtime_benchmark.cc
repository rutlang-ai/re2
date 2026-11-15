#include <stdio.h>
#include <time.h>

#include "re2/re2.h"
#include "re2/set.h"
#ifdef BASELINE
using String = std::string;
using View = absl::string_view;
using Ints = std::vector<int>;
#else
using String = re2::String;
using View = re2::StringView;
using Ints = re2::Vector<int>;
#endif
static double now() {
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}
volatile int sink;
int main() {
  const char* patterns[] = {"hello world", "([a-z]+)-(\\d+)",
                            "foo.*bar|baz[0-9]+", "(a|b|ab)*z", "\\p{Greek}+"};
  const char* texts[] = {"hello world", "hello-123",
                         "prefix foo some longer middle text bar suffix",
                         "ababababababababababababababababz", "αβγδεζηθ"};
  for (int c = 0; c < 5; ++c) {
    RE2 re(patterns[c]);
    int count = 0;
    double t = now();
    for (int i = 0; i < 1000000; ++i) count += RE2::PartialMatch(texts[c], re);
    sink = count;
    printf("match%d %.3f\n", c, (now() - t) * 1e9 / 1000000);
  }
  {
    RE2 re("([a-z]+)-(\\d+)");
    int count = 0, n = 0;
    String s;
    double t = now();
    for (int i = 0; i < 500000; ++i)
      count += RE2::FullMatch("hello-123", re, &s, &n);
    sink = count;
    printf("capture %.3f\n", (now() - t) * 1e9 / 500000);
  }
  {
    RE2 re("([a-z]+)-(\\d+)");
    int count = 0, n = 0;
    View s;
    double t = now();
    for (int i = 0; i < 500000; ++i)
      count += RE2::FullMatch("hello-123", re, &s, &n);
    sink = count;
    printf("capture_view %.3f\n", (now() - t) * 1e9 / 500000);
  }
  for (int c = 0; c < 5; ++c) {
    double t = now();
    int count = 0;
    for (int i = 0; i < 5000; ++i) {
      RE2 re(patterns[c]);
      count += re.ok();
    }
    sink = count;
    printf("compile%d %.3f\n", c, (now() - t) * 1e9 / 5000);
  }
  {
    RE2::Set set(RE2::DefaultOptions, RE2::UNANCHORED);
    set.Add("foo.*bar", nullptr);
    set.Add("[0-9]+", nullptr);
    set.Add("hello", nullptr);
    set.Compile();
    Ints ids;
    double t = now();
    int count = 0;
    for (int i = 0; i < 300000; ++i) count += set.Match("foo 123 bar", &ids);
    sink = count;
    printf("set %.3f\n", (now() - t) * 1e9 / 300000);
  }
}
