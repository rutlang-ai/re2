# RE2, a regular expression library

RE2 is an efficient, principled regular expression library
that has been used in production at Google and many other places
since 2006.

_**Safety is RE2's primary goal.**_

RE2 was designed and implemented with an explicit goal of being able
to handle regular expressions from untrusted users without risk.
One of its primary guarantees is that the match time is linear in the
length of the input string. It was also written with production concerns in mind:
the parser, the compiler and the execution engines limit their memory usage
by working within a configurable budget—failing gracefully when exhausted—and
they avoid stack overflow by eschewing recursion.

It is not a goal to be faster than all other engines under all circumstances.
Although RE2 guarantees a running time that is asymptotically linear in
the length of the input, more complex expressions may incur larger constant factors;
longer expressions increase the overhead required to handle those expressions safely.
In a sense, RE2 is pessimistic where a backtracking engine is optimistic:
A backtracking engine tests each alternative sequentially, making it fast when the first alternative is common.
By contrast RE2 evaluates all alternatives in parallel, avoiding the performance penalty for the last alternative,
at the cost of some overhead. This pessimism is what makes RE2 secure.

It is also not a goal to implement all of the features offered by Perl, PCRE and other engines.
As a matter of principle, RE2 does not support constructs for which only backtracking solutions are known to exist.
Thus, backreferences and look-around assertions are not supported.

For more information, please refer to Russ Cox's articles on regular expression theory and practice:

* [Regular Expression Matching Can Be Simple And Fast](https://swtch.com/~rsc/regexp/regexp1.html)
* [Regular Expression Matching: the Virtual Machine Approach](https://swtch.com/~rsc/regexp/regexp2.html)
* [Regular Expression Matching in the Wild](https://swtch.com/~rsc/regexp/regexp3.html)

### Syntax

In POSIX mode, RE2 accepts standard POSIX (egrep) syntax regular expressions.
In Perl mode, RE2 accepts most Perl operators.  The only excluded ones are
those that require backtracking (and its potential for exponential runtime)
to implement.  These include backreferences (submatching is still okay)
and generalized assertions.
The [Syntax wiki page](https://github.com/google/re2/wiki/Syntax)
documents the supported Perl-mode syntax in detail.
The default is Perl mode.

### C++ API

RE2's native language is C++, although there are [ports and wrappers](#ports-and-wrappers) listed below.

#### Matching Interface

There are two basic operators:
`RE2::FullMatch` requires the regexp to match the entire input text, and
`RE2::PartialMatch` looks for a match for a substring of the input text,
returning the leftmost-longest match in POSIX mode and the
same match that Perl would have chosen in Perl mode.

Examples:

```cpp
assert(RE2::FullMatch("hello", "h.*o"))
assert(!RE2::FullMatch("hello", "e"))

assert(RE2::PartialMatch("hello", "h.*o"))
assert(RE2::PartialMatch("hello", "e"))
```

#### Submatch Extraction

Both matching functions take additional arguments in which submatches will be stored.
The argument can be a `string*`, or an integer type, or the type `re2::StringView*`.
(The `re2::StringView` type is very similar to the `std::string_view` type,
but for historical reasons, RE2 uses the former.)
A `string_view` is a pointer to the original input text, along with a count.
It behaves like a string but doesn't carry its own storage.
Like when using a pointer, when using a `string_view`
you must be careful not to use it once the original text has been deleted or gone out of scope.

Examples:

```cpp
// Successful parsing.
int i;
string s;
assert(RE2::FullMatch("ruby:1234", "(\\w+):(\\d+)", &s, &i));
assert(s == "ruby");
assert(i == 1234);

// Fails: "ruby" cannot be parsed as an integer.
assert(!RE2::FullMatch("ruby", "(.+)", &i));

// Success; does not extract the number.
assert(RE2::FullMatch("ruby:1234", "(\\w+):(\\d+)", &s));

// Success; skips NULL argument.
assert(RE2::FullMatch("ruby:1234", "(\\w+):(\\d+)", (void*)NULL, &i));

// Fails: integer overflow keeps value from being stored in i.
assert(!RE2::FullMatch("ruby:123456789123", "(\\w+):(\\d+)", &s, &i));
```

#### Pre-Compiled Regular Expressions

The examples above all recompile the regular expression on each call.
Instead, you can compile it once to an RE2 object and reuse that object for each call.

Example:
```cpp
RE2 re("(\\w+):(\\d+)");
assert(re.ok());  // compiled; if not, see re.error();

assert(RE2::FullMatch("ruby:1234", re, &s, &i));
assert(RE2::FullMatch("ruby:1234", re, &s));
assert(RE2::FullMatch("ruby:1234", re, (void*)NULL, &i));
assert(!RE2::FullMatch("ruby:123456789123", re, &s, &i));
```

#### Options

The constructor takes an optional second argument that can
be used to change RE2's default options.
For example, `RE2::Quiet` silences the error messages that are
usually printed when a regular expression fails to parse:

```cpp
RE2 re("(ab", RE2::Quiet);  // don't write to stderr for parser failure
assert(!re.ok());  // can check re.error() for details
```

Other useful predefined options are `Latin1` (disable UTF-8) and `POSIX`
(use POSIX syntax and leftmost longest matching).

You can also declare your own `RE2::Options` object and then configure it as you like.
See the [header](https://github.com/google/re2/blob/main/re2/re2.h) for the full set of options.

#### Unicode Normalization

RE2 operates on Unicode code points: it makes no attempt at normalization.
For example, the regular expression /ü/ (U+00FC, u with diaeresis)
does not match the input "ü" (U+0075 U+0308, u followed by combining diaeresis).
Normalization is a long, involved topic.
The simplest solution, if you need such matches, is to normalize both the regular expressions
and the input in a preprocessing step before using RE2.
For more details on the general topic, see <https://www.unicode.org/reports/tr15/>.

#### Additional Tips and Tricks

For advanced usage, like constructing your own argument lists,
or using RE2 as a lexer, or parsing hex, octal, and C-radix numbers,
see [re2.h](https://github.com/google/re2/blob/main/re2/re2.h).

### Installation

RE2 can be built and installed using GNU make, CMake, or Bazel.
The simplest installation instructions are:

	make
	make test
	make benchmark
	make install
	make testinstall

This fork requires a C++20 compiler and POSIX (tested on Linux with GCC and Clang).
It does not link Abseil, libstdc++, libc++, libsupc++ or libc++abi. Standard library
headers are still used for facilities that compile entirely into the caller, such
as type traits and algorithms. Containa, SmallString and the fmt
headers required by SmallString are vendored in `third_party/`; no dependency
fetch is needed. Built-in Unicode tables remain available; the optional ICU C++
backend is disabled because it would reintroduce a C++ runtime dependency.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The default build includes `re2_no_stdlib_test`, a consumer linked without a C++
standard library. On Linux, a second test checks both the executable and library
for forbidden dependencies and unresolved runtime symbols. Run the same commands
with `-DBUILD_SHARED_LIBS=OFF` to verify a static archive. The shared library keeps
its small malloc/free allocation ABI and static initialization guards private.

The API/ABI changes in this fork require consumers to rebuild (SONAME 12):

| Previous type | Replacement |
| --- | --- |
| `std::string` | `re2::String` (`small::small_string`) |
| `absl::string_view` | `re2::StringView` (also `re2::StringPiece`) |
| `std::vector<T>` | `re2::Vector<T>` (Containa `small_vectra`) |
| `std::map<K,V>` | `re2::Map<K,V>` (Containa `btree_map`) |

`StringView` accepts pointer/length pairs and existing string-like objects without
copying. Captures that own their bytes use `re2::String`. B-tree mutation can
invalidate iterators and references. SmallString has a 32-bit size limit; consult
its upstream documentation before passing multi-gigabyte patterns or captures.

A C++ compiler driver normally adds a standard library even if the code does not
use it. For standalone consumers use `clang++ -std=c++20 -fno-exceptions -fno-rtti
-nostdlib++ ... -lre2 -lm -pthread`, or GCC's `-nodefaultlibs` with `-lm -lc -lgcc_s
-lgcc`. In unoptimized builds using libstdc++ headers also define
`_GLIBCXX_NO_ASSERTIONS` to avoid its out-of-line assertion helper; CMake propagates
this definition. RE2 and vendor assertions remain enabled in Debug builds.
Allocation failure terminates the process; there are no C++ exception paths.

To run the original tests, install GoogleTest and configure with `-DRE2_TEST=ON`.
The optional upstream benchmarks additionally need Google Benchmark and
`-DRE2_BENCHMARK=ON`. Test, benchmark and Python/pybind11/Embind executables can use their
own C++ runtime; they are separate from the runtime-free library and standalone
consumer test. The Python wrapper converts the new container types explicitly.

See [the migration and benchmark notes](doc/runtime-free.md) for reproducible
performance checks and the recorded comparison.

Use `add_subdirectory()` or an installed `find_package(re2)` target, then
`target_link_libraries(your_target PRIVATE re2::re2)`. The installed package includes
all required headers and does not find or link Abseil.

If you are using [Bazel](https://bazel.io), it will handle the dependencies for you,
although you still need to download Bazel,
which you can do with [Bazelisk](https://github.com/bazelbuild/bazelisk).

	go install github.com/bazelbuild/bazelisk@latest
	# or on mac: brew install bazelisk

	bazelisk build :all
	bazelisk test :all

If you are using RE2 from another project, you need to make sure you are
using at least C++17.
See the RE2 [.bazelrc](https://github.com/google/re2/blob/main/.bazelrc) file for an example.

### Ports and Wrappers

RE2 is implemented in C++.

The official Python wrapper is [in the `python` directory](https://github.com/google/re2/tree/main/python)
and [published on PyPI as `google-re2`](https://pypi.org/project/google-re2/).
Note that there is also a PyPI `re2` but it is not by the RE2 authors and is unmaintained. Use `google-re2`.

There are also other unofficial wrappers:

- A C wrapper is at <https://github.com/marcomaggi/cre2/>.
- A D wrapper is at <https://github.com/ShigekiKarita/re2d/> and [on DUB](https://code.dlang.org/packages/re2d).
- An Erlang wrapper is at <https://github.com/dukesoferl/re2/> and [on Hex](https://hex.pm/packages/re2).
- An Inferno wrapper is at <https://github.com/powerman/inferno-re2/>.
- A Node.js wrapper is at <https://github.com/uhop/node-re2/> and [on NPM](https://www.npmjs.com/package/re2).
- An OCaml wrapper is at <https://github.com/janestreet/re2/> and [on OPAM](https://opam.ocaml.org/packages/re2/).
- A Perl wrapper is at <https://github.com/dgl/re-engine-RE2/> and [on CPAN](https://metacpan.org/pod/re::engine::RE2).
- An R wrapper is at <https://github.com/girishji/re2/> and [on CRAN](https://cran.r-project.org/web/packages/re2/index.html).
- A Ruby wrapper is at <https://github.com/mudge/re2/> and on RubyGems (rubygems.org).
- A WebAssembly wrapper is at <https://github.com/google/re2-wasm/> and on NPM (npmjs.com).

[RE2J](https://github.com/google/re2j) is a port of the RE2 C++ code to pure Java,
and [RE2JS](https://github.com/le0pard/re2js) is a port of RE2J to JavaScript.

The [Go `regexp` package](https://go.dev/pkg/regexp)
and [Rust `regex` crate](https://docs.rs/regex)
do not share code with RE2, but they follow the same principles,
accept the same syntax, and provide the same efficiency guarantees.

### Contact

The [issue tracker](https://github.com/google/re2/issues) is the best place for discussions.

There is a [mailing list](https://groups.google.com/group/re2-dev) for keeping up with code changes.

Please read the [contribution guide](https://github.com/google/re2/wiki/Contribute) before sending changes.
In particular, note that RE2 does not use GitHub pull requests.
