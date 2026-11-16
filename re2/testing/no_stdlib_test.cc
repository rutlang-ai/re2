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

static int CheckAliasedVectorResize() {
  for (size_t initial : {size_t(1), size_t(2), size_t(8)}) {
    re2::Vector<int> values(initial, 42);
    values.resize(100, values[initial - 1]);
    VERIFY(values.size() == 100);
    for (int value : values) VERIFY(value == 42);
    re2::Vector<re2::String> strings(initial, re2::String(100, 'x'));
    strings.resize(100, strings[0]);
    for (const auto& value : strings) VERIFY(value == re2::String(100, 'x'));
  }
  return 0;
}

static int CheckAliasedVectorInsert() {
  for (bool reserve : {false, true}) {
    re2::Vector<int> values{11, 42};
    if (reserve) values.reserve(16);
    auto result = values.insert(values.begin(), values[1]);
    VERIFY(result == values.begin());
    VERIFY(values.size() == 3);
    VERIFY(values[0] == 42 && values[1] == 11 && values[2] == 42);
    re2::Vector<re2::String> strings{re2::String(100, 'a'), re2::String(100, 'b')};
    if (reserve) strings.reserve(16);
    strings.insert(strings.begin(), strings[1]);
    VERIFY(strings[0] == re2::String(100, 'b'));
    VERIFY(strings[1] == re2::String(100, 'a'));
    VERIFY(strings[2] == re2::String(100, 'b'));
  }
  return 0;
}

static int CheckAliasedVectorAssign() {
  for (size_t initial : {size_t(1), size_t(2), size_t(8)}) {
    for (size_t count : {size_t(0), size_t(1), size_t(8), size_t(100)}) {
      re2::Vector<re2::String> values(initial, re2::String(100, 'x'));
      values.assign(count, values[initial - 1]);
      VERIFY(values.size() == count);
      for (const auto& value : values) VERIFY(value == re2::String(100, 'x'));
      re2::Vector<int> ints(initial, 42);
      ints.assign(count, ints[0]);
      VERIFY(ints.size() == count);
      for (int value : ints) VERIFY(value == 42);
    }
  }
  return 0;
}

// Copies share a cursor, just as stream iterators share their input stream.
struct SinglePassChars {
  using value_type = char;
  using difference_type = ptrdiff_t;
  using pointer = const char*;
  using reference = const char&;
  using iterator_category = std::input_iterator_tag;
  const char** cursor;
  const char* end;
  bool sentinel;
  reference operator*() const { return **cursor; }
  SinglePassChars& operator++() { ++*cursor; return *this; }
  SinglePassChars operator++(int) { auto copy = *this; ++*this; return copy; }
  friend bool operator==(SinglePassChars a, SinglePassChars b) {
    return a.cursor == b.cursor &&
           (a.sentinel == b.sentinel || *a.cursor == a.end);
  }
};
static_assert(std::input_iterator<SinglePassChars>);
static_assert(!std::forward_iterator<SinglePassChars>);

static int CheckSinglePassStringConstruction() {
  for (size_t length : {size_t(0), size_t(3), size_t(9), size_t(100), size_t(16384)}) {
    re2::String expected(length, 'x');
    if (length > 2) expected[1] = '\0';
    const char* cursor = expected.data();
    re2::String value(SinglePassChars{&cursor, expected.data() + length, false},
                      SinglePassChars{&cursor, expected.data() + length, true});
    VERIFY(value == expected);
    VERIFY(cursor == expected.data() + length);
    VERIFY(value.c_str()[length] == '\0');
  }
  return 0;
}

static int CheckAliasedStringIteratorAssign() {
  for (size_t length : {size_t(6), size_t(100), size_t(16384)}) {
    re2::String original(length, 'x');
    for (size_t i = 0; i < length; ++i) original[i] = static_cast<char>('a' + i % 23);
    re2::String value = original;
    value.assign(value.rbegin(), value.rend());
    VERIFY(value.size() == original.size());
    for (size_t i = 0; i < length; ++i) VERIFY(value[i] == original[length - i - 1]);
    value.assign(value.begin() + 1, value.end());
    VERIFY(value.size() == length - 1);
    for (size_t i = 0; i < value.size(); ++i) VERIFY(value[i] == original[length - i - 2]);
  }
  re2::String original("a\0bcdefgh", 9);
  const char* cursor = original.data();
  re2::String value("old");
  value.assign(SinglePassChars{&cursor, original.data() + original.size(), false},
               SinglePassChars{&cursor, original.data() + original.size(), true});
  VERIFY(value == original);
  return 0;
}

int main() {
  VERIFY(CheckAliasedStringIteratorAssign() == 0);
  VERIFY(CheckSinglePassStringConstruction() == 0);
  VERIFY(CheckAliasedVectorAssign() == 0);
  VERIFY(CheckAliasedVectorInsert() == 0);
  VERIFY(CheckAliasedVectorResize() == 0);
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
