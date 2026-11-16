Vendored header dependencies (no external downloads are needed to build RE2):

- Containa: https://github.com/clapdb/Containa at bff9668b14f7d57ddbdb23cc6fad6f71e07f84aa.
  Only the seven used container headers are included. Local changes replace throw paths with
  abort, remove unused exception rollback code and iostream debug logging, and make B-tree
  three-way search honor custom comparators. RE2 supplies a malloc-based allocator.
  The aliased resize fill-value fix is included upstream via
  [clapdb/Containa#67](https://github.com/clapdb/Containa/pull/67).
  The aliased insertion-value fix is included upstream via
  [clapdb/Containa#68](https://github.com/clapdb/Containa/pull/68).
  The aliased assignment fill-value fix is included upstream via
  [clapdb/Containa#69](https://github.com/clapdb/Containa/pull/69).
- SmallString: https://github.com/clapdb/smallstring at 2b2ae5d291184c946b9faa27d49e536d194fe5b9.
  Local changes replace throw paths with abort, constrain iterator overloads to non-integral
  types, check malloc failures, retain the inline-size invariant in Release, accelerate
  inline capture assignment and use the vendored fmt include path. The malloc-backed small_string is used.
  The character-search bounds and aliased-append safety fixes are included upstream via
  [clapdb/smallstring#13](https://github.com/clapdb/smallstring/pull/13) and
  [clapdb/smallstring#14](https://github.com/clapdb/smallstring/pull/14).
  The aliased-insertion and string-view replacement fixes are also included upstream via [clapdb/smallstring#15](https://github.com/clapdb/smallstring/pull/15) and
  [clapdb/smallstring#16](https://github.com/clapdb/smallstring/pull/16).
  The single-pass iterator construction fix is included upstream via
  [clapdb/smallstring#17](https://github.com/clapdb/smallstring/pull/17).
  The aliased iterator assignment fix is included upstream via
  [clapdb/smallstring#18](https://github.com/clapdb/smallstring/pull/18).
- fmt: https://github.com/fmtlib/fmt at 9908d00037d58233bcb1a7c99153bb96363994df.
  Only base.h, format.h and format-inl.h are needed by SmallString's formatter declarations.
  RE2 does not instantiate fmt formatting functions; its diagnostics use C stdio.

Each dependency retains its upstream license beside its headers. Updates should rerun the
standalone link test, upstream RE2 tests, sanitizers and the performance comparison.
