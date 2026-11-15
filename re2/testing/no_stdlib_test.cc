// This executable is linked without any C++ standard library or ABI library.
#include <pthread.h>
#include <stdio.h>

#include "re2/filtered_re2.h"
#include "re2/re2.h"
#include "re2/set.h"

#define VERIFY(c)                                             \
  do {                                                        \
    if (!(c)) {                                               \
      fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); \
      return 1;                                               \
    }                                                         \
  } while (0)

static re2::LazyRE2 shared = {"(?P<word>[a-z]+)-(\\d+)"};
static void* MatchThread(void*) {
  for (int i = 0; i < 1000; ++i) {
    re2::String word;
    int number = 0;
    if (!RE2::FullMatch("hello-123", *shared, &word, &number) ||
        word != "hello" || number != 123)
      return reinterpret_cast<void*>(1);
    if (shared->NamedCapturingGroups().size() != 1)
      return reinterpret_cast<void*>(1);
  }
  return nullptr;
}
int main() {
  re2::String text = "hello-123";
  re2::String word;
  int number = 0;
  VERIFY(RE2::FullMatch(text, "([a-z]+)-(\\d+)", &word, &number));
  VERIFY(word == "hello" && number == 123);
  VERIFY(RE2::Replace(&text, "([a-z]+)-(\\d+)", "\\2/\\1"));
  VERIFY(text == "123/hello");
  VERIFY(RE2::GlobalReplace(&text, "[a-z]", "x") == 5);
  VERIFY(text == "123/xxxxx");
  RE2 bad("[");
  VERIFY(!bad.ok());
  RE2 unicode("\\p{Greek}+");
  VERIFY(RE2::FullMatch("αβγ", unicode));
  re2::RE2::Set set(RE2::DefaultOptions, RE2::UNANCHORED);
  VERIFY(set.Add("hello", nullptr) == 0);
  VERIFY(set.Add("[0-9]+", nullptr) == 1);
  VERIFY(set.Compile());
  re2::Vector<int> ids;
  VERIFY(set.Match("hello-123", &ids));
  VERIFY(ids.size() == 2);
  re2::FilteredRE2 filtered;
  int id = -1;
  VERIFY(filtered.Add("hello.*", RE2::DefaultOptions, &id) == RE2::NoError);
  re2::Vector<re2::String> atoms;
  filtered.Compile(&atoms);
  ids.clear();
  for (size_t i = 0; i < atoms.size(); ++i)
    if (re2::String("hello").find(atoms[i]) != re2::String::npos)
      ids.push_back(i);
  VERIFY(filtered.FirstMatch("hello world", ids) == 0);
  pthread_t threads[8];
  for (auto& t : threads)
    VERIFY(pthread_create(&t, nullptr, MatchThread, nullptr) == 0);
  for (auto& t : threads) {
    void* result = nullptr;
    VERIFY(pthread_join(t, &result) == 0);
    VERIFY(result == nullptr);
  }
  // LazyRE2 deliberately retains its object for process lifetime. Free this
  // test-owned instance: LSan cannot follow SmallString's tagged heap pointer.
  shared.ptr_->~RE2();
  free(shared.ptr_);
  shared.ptr_ = nullptr;
  puts("RE2 works without a C++ standard library");
  return 0;
}
