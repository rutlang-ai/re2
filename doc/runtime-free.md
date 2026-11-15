# Runtime-free RE2

This fork removes the link dependencies on Abseil and all C++ standard-library
runtimes. It retains compiler-provided C++20 headers for facilities whose code can
be instantiated locally. Linux shared objects have only libc, libm and the system
loader in their dependency closure. Neither a static C++ runtime nor a copy of
Abseil is bundled in the result.

Owning strings use clapdb/SmallString and containers use clapdb/Containa. Their
pinned revisions, licenses and local patches are documented in
[third_party/README.md](../third_party/README.md). Public signatures use
`re2::String`, `StringView`, `Vector` and `Map`. This changes the ABI (SONAME 12)
and requires consumers to rebuild. SmallString's size limit is checked before
pattern construction and capture assignment.

The small allocation ABI in `re2/runtime.cc` forwards new/delete to malloc/free.
It also provides static-initialization guards; it is not a linked C++ runtime.
On ELF shared builds these entry points are local, so they do not replace the
host application's allocation functions. In a standalone static application they
supply the allocation ABI. Allocation failure aborts. Callers that use global
`new` themselves must also supply that ABI; stack objects and the provided
containers do not require the host to link a C++ standard library.

DFA cache readers use an inline atomic count; a pthread mutex serializes writers.
Writers announce themselves before draining active readers, preventing new readers
from starving the current writer. Failed readers can transiently increment the
count, so writer unlock preserves those increments until their owners back out.
`rw_mutex_test` exercises readers and writers concurrently and can be compiled
with ThreadSanitizer.

Boolean matches skip capture-storage construction. Anchored OnePass matches skip
DFA bookkeeping under the same selection conditions as upstream. Decimal integer
captures use header-only integral `from_chars`, accepting a leading `+` as before.
Other radices retain the original C parsing path. The original integer boundary
tests and additional bounded-input/sign/leading-zero cases cover this change.

## Verification

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The default build links two consumers with `-nostdlib++` on Clang, or
`-nodefaultlibs` and explicit C/compiler libraries on GCC. Linker errors reject
unresolved symbols. The dependency test checks both direct and transitive ELF
dependencies and unresolved dynamic symbols. Repeat with `BUILD_SHARED_LIBS=OFF`
for static-library validation. Both GCC and Clang were checked, as was compiling
against installed headers with no standard C++ runtime.

Optional upstream GTest, benchmark, Python and Embind programs have their own
runtime requirements; those requirements are not dependencies of `libre2`.
Validation includes all 20 upstream C++ test executables (including the four
exhaustive suites), the three standalone checks, ASan/UBSan on the non-exhaustive
suite, a TSan reader/writer stress run, Bazel's small tests and 243 Python tests.
The optional upstream Google Benchmark target also builds and runs.

## Performance comparison

Recorded on 2026-11-15. The comparison uses upstream commit `2da0056814cf180480a19f5cf811e7e1c054bf6d`
and this working tree, Clang 22.1.8, Release optimization and an AMD Ryzen 9 5950X.
Both microbenchmark programs link static RE2 archives. The candidate links no C++
standard library. Each pair runs in alternating order on the same CPU; the table
reports medians of 21 samples in nanoseconds. Negative changes indicate less time.
Raw samples are in [runtime-free.json](../benchlog/runtime-free.json).

| Case | Baseline ns | Candidate ns | Change |
| --- | ---: | ---: | ---: |
| match0 | 44.60 | 39.77 | -10.8% |
| match1 | 31.75 | 30.51 | -3.9% |
| match2 | 70.17 | 69.34 | -1.2% |
| match3 | 66.12 | 63.07 | -4.6% |
| match4 | 27.52 | 24.62 | -10.5% |
| capture | 56.90 | 50.41 | -11.4% |
| capture_view | 52.80 | 46.20 | -12.5% |
| compile0 | 1930.72 | 1747.48 | -9.5% |
| compile1 | 2222.64 | 2053.10 | -7.6% |
| compile2 | 4115.41 | 3752.85 | -8.8% |
| compile3 | 2211.82 | 2076.09 | -6.1% |
| compile4 | 14516.00 | 12564.78 | -13.4% |
| set | 62.60 | 53.77 | -14.1% |

`match0` through `match4` cover a literal, captured ASCII fields, alternation,
repetition and a Unicode property. `capture` copies a string and parses an integer;
`capture_view` returns a view and parses an integer. `compile0` through `compile4`
construct the corresponding regexps. `set` searches three patterns together.

The upstream `Search_Easy0_CachedDFA` benchmark additionally checks 8-byte and
512-byte inputs with one and four threads. Nine alternating runs of static-library binaries were pinned to
the same four CPUs. The raw median-CPU-time samples are in
[runtime-free-threaded.json](../benchlog/runtime-free-threaded.json).

| Input / threads | Baseline ns | Candidate ns | Change |
| --- | ---: | ---: | ---: |
| 8/threads:1 | 20.54 | 20.10 | -2.1% |
| 8/threads:4 | 129.98 | 99.48 | -23.5% |
| 512/threads:1 | 35.49 | 32.49 | -8.5% |
| 512/threads:4 | 199.61 | 93.70 | -53.1% |

These are sampled workloads, not a guarantee for every regexp or machine. CPU
frequency scaling and sub-nanosecond differences can affect tiny cases; retain
the raw samples when comparing future changes.

To reproduce the candidate microbenchmark:

```sh
cmake -S . -B build-perf -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF -DRE2_RUNTIME_BENCHMARK=ON
cmake --build build-perf -j
python3 tools/compare_runtime_benchmark.py /path/to/baseline-benchmark \
  build-perf/re2_runtime_benchmark --runs 21 --output comparison.json
```

Build the baseline from the commit above with the same compiler and optimization,
then compile `re2/testing/runtime_benchmark.cc` with `-DBASELINE`, that checkout's
headers/static archive and its Abseil link flags. The `BASELINE` switch adapts only
the container/string type names; the measured operations and iteration counts
are identical. Run performance comparisons serially, separately from test suites.
