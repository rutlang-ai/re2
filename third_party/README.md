Vendored header dependencies (no external downloads are needed to build RE2):

- Containa: https://github.com/clapdb/Containa at f8cae273839d29d4136f3787373cfde7ea983b5a.
  Only the seven used container headers are included. Local changes replace throw paths with
  abort, remove unused exception rollback code and iostream debug logging, and make B-tree
  three-way search honor custom comparators. RE2 supplies a malloc-based allocator.
- SmallString: https://github.com/clapdb/smallstring at 659429fad0900198fbba9805bfb9e7f6c0d1beeb.
  Local changes replace throw paths with abort, constrain iterator overloads to non-integral
  types, check malloc failures, retain the inline-size invariant in Release, accelerate
  inline capture assignment and use the vendored fmt include path. The malloc-backed small_string is used.
  The character-search bounds fix is included upstream via [clapdb/smallstring#13](https://github.com/clapdb/smallstring/pull/13).
- fmt: https://github.com/fmtlib/fmt at 9908d00037d58233bcb1a7c99153bb96363994df.
  Only base.h, format.h and format-inl.h are needed by SmallString's formatter declarations.
  RE2 does not instantiate fmt formatting functions; its diagnostics use C stdio.

Each dependency retains its upstream license beside its headers. Updates should rerun the
standalone link test, upstream RE2 tests, sanitizers and the performance comparison.
