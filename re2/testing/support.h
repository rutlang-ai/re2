#ifndef RE2_TESTING_SUPPORT_H_
#define RE2_TESTING_SUPPORT_H_
#include <array>
#include <ostream>

#include "re2/support.h"
namespace re2 {
inline std::ostream& operator<<(std::ostream& os, StringView s) {
  return os.write(s.data() ? s.data() : "", s.size());
}
inline std::ostream& operator<<(std::ostream& os, const String& s) {
  return os << StringView(s);
}
namespace internal {
template <class T>
const T& GetFlag(const T& t) {
  return t;
}
inline String CEscape(StringView s) {
  String out;
  for (unsigned char c : s) {
    if (c == '\n')
      out += "\\n";
    else if (c == '\r')
      out += "\\r";
    else if (c == '\t')
      out += "\\t";
    else if (c == '\\')
      out += "\\\\";
    else if (c == '\"')
      out += "\\\"";
    else if (c == '\'')
      out += "\\'";
    else if (c >= 32 && c < 127)
      out += c;
    else {
      char b[5];
      snprintf(b, sizeof(b), "\\%03o", c);
      out += b;
    }
  }
  return out;
}
template <char... C>
class ParsedFormat {
 public:
  static std::unique_ptr<ParsedFormat> New(StringView f) {
    size_t count = 0;
    for (size_t i = 0; i < f.size(); ++i)
      if (f[i] == '%') {
        if (++i == f.size()) return nullptr;
        if (f[i] == '%') continue;
        if (f[i] != 's') return nullptr;
        ++count;
      }
    if (count != sizeof...(C)) return nullptr;
    return std::unique_ptr<ParsedFormat>(new ParsedFormat(f));
  }
  StringView text() const { return StringView(f_); }

 private:
  explicit ParsedFormat(StringView f) : f_(f) {}
  String f_;
};
template <char... C, class... T>
String StrFormat(const ParsedFormat<C...>& f, const T&... v) {
  static_assert(sizeof...(C) == sizeof...(T));
  const std::array<StringView, sizeof...(T)> args = {StringView(v)...};
  String result;
  StringView format = f.text();
  size_t arg = 0;
  for (size_t i = 0; i < format.size(); ++i) {
    if (format[i] != '%') result += format[i];
    else if (format[++i] == '%') result += '%';
    else {
      StringView value = args[arg++];
      if (!value.empty()) result.append(value.data(), value.size());
    }
  }
  return result;
}
}  // namespace internal
}  // namespace re2
#define RE2_FLAG(type, name, value, help) static type FLAGS_##name = value
#define RE2_VLOG(n) \
  if (true) {       \
  } else            \
    RE2_LOG(ERROR)
#endif
