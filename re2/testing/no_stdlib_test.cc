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
static int CheckStringCharacterFind() {
  using re2::String;
  auto quoted = RE2::QuoteMeta("abc");
  VERIFY(quoted.find('\0', 4) == String::npos);
  VERIFY(quoted.find('b') == 1);
  VERIFY(quoted.find('b', 1) == 1);
  VERIFY(quoted.find('b', 2) == String::npos);

  // Exercise empty, inline, short and larger heap-backed strings.
  const size_t lengths[] = {0, 3, 6, 7, 255, 256, 16384};
  for (size_t length : lengths) {
    String str(length, 'a');
    const size_t positions[] = {length, length + 1,
                                static_cast<size_t>(String::npos),
                                static_cast<size_t>(-1)};
    for (size_t pos : positions) {
      VERIFY(str.find('\0', pos) == String::npos);
      VERIFY(str.find('a', pos) == String::npos);
      VERIFY(str.find('z', pos) == String::npos);
    }
    if (length != 0) {
      str[length - 1] = 'z';
      VERIFY(str.find('z') == length - 1);
      VERIFY(str.find('z', length - 1) == length - 1);
    }
  }
  String binary("a\0b", 3);
  VERIFY(binary.find('\0') == 1);
  VERIFY(binary.find('\0', 1) == 1);
  VERIFY(binary.find('\0', 2) == String::npos);
  return 0;
}

static int CheckAliasedAppend() {
  re2::String str(100, 'x');
  str.append(str.data(), str.size());
  VERIFY(str == re2::String(200, 'x'));
  str = re2::String(100, 'y');
  str.append(re2::StringView(str));
  VERIFY(str == re2::String(200, 'y'));
  str = re2::String(100, 'z');
  str.append(str.begin(), str.end());
  VERIFY(str == re2::String(200, 'z'));
  str = re2::String("abcdef");
  str.append(str.rbegin(), str.rend());
  VERIFY(str == "abcdeffedcba");
  str = re2::String("abcdef");
  str.append(str.data(), str.size() + 1);
  VERIFY(str == re2::String("abcdefabcdef\0", 13));
  return 0;
}

static int CheckAliasedInsert() {
  re2::String str(100, 'x');
  str.insert(size_t{0}, str.data(), str.size());
  VERIFY(str == re2::String(200, 'x'));
  str = re2::String("abcdef");
  str.reserve(32);
  str.insert(size_t{1}, str.data() + 2, 3);
  VERIFY(str == "acdebcdef");
  str = re2::String(100, 'y');
  str.insert(size_t{0}, str);
  VERIFY(str == re2::String(200, 'y'));
  str = re2::String("abcdef");
  auto inserted = str.insert(str.begin() + 2, str.begin() + 1, str.end());
  VERIFY(str == "abbcdefcdef");
  VERIFY(inserted == str.begin() + 2);
  str = re2::String("abcdef");
  inserted = str.insert(str.begin() + 2, str.rbegin(), str.rend());
  VERIFY(str == "abfedcbacdef");
  VERIFY(inserted == str.begin() + 2);
  return 0;
}

static int CheckViewReplaceOffsets() {
  re2::String str("abcdef");
  str.replace(1, 2, re2::StringView("XYZ"), 0, 2);
  VERIFY(str == "aXYdef");
  str = re2::String("abcdefghij");
  str.replace(8, 2, re2::StringView("XYZ"), 1, 2);
  VERIFY(str == "abcdefghYZ");
  str = re2::String("abcdef");
  str.replace(1, 2, re2::StringView("XYZ"), 1, 100);
  VERIFY(str == "aYZdef");
  str = re2::String("abcdef");
  str.replace(1, 2, re2::StringView(str), 0, 2);
  VERIFY(str == "aabdef");
  str.replace(1, 2, re2::StringView(), 0, 0);
  VERIFY(str == "adef");
  return 0;
}

int main() {
  VERIFY(CheckStringCharacterFind() == 0);
  VERIFY(CheckAliasedAppend() == 0);
  VERIFY(CheckAliasedInsert() == 0);
  VERIFY(CheckViewReplaceOffsets() == 0);
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
