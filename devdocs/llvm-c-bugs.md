# Known LLVM-C API Bugs and Limitations

Last updated: February 19, 2026

This document tracks issues relevant to llvm-c-test parity work.

## 1. Poison/Undef Predicate Ordering in Echo (Historical)

- Status: Fixed in both vendored C llvm-c-test and Python implementation.
- Scope: `--echo`
- Root cause: `LLVMIsUndef(poison)` is true, so checking `undef` before
  `poison` misclassifies poison constants.
- Fix summary: check poison before undef in constant cloning.
- Lit coverage: `llvm-c/llvm-c-test/inputs/poison.ll`

## 2. BitCast Echo Crash (Historical)

- Status: Not reproducible on current toolchain (LLVM 21.1.1 in this repo).
- Scope: `--echo`
- Action taken: Added regression coverage to ensure bitcast round-trips.
- Lit coverage: `llvm-c/llvm-c-test/inputs/echo_bitcast.ll`

## 3. ConstantFP Cloning Support (Previously Missing)

- Status: Fixed in both vendored C llvm-c-test and Python implementation.
- Scope: `--echo`
- Fix summary:
  - Exact bit-pattern preserving clone for hex FP literals.
  - Handles `0xH`, `0xR`, `0xK`, `0xL`, `0xM`, and plain `0x...` forms.
- Lit coverage: `llvm-c/llvm-c-test/inputs/echo_constant_fp.ll`

## 4. BFloat Type Cloning (Previously Incorrect in C)

- Status: Fixed.
- Scope: `--echo`
- Lit coverage: `llvm-c/llvm-c-test/inputs/types_extended.ll`

## 5. Object Symbol Size Crash in C llvm-c-test

- Status: Fixed in vendored `llvm-c-test` code.
- Scope: `--object-list-symbols`
- Root cause: C test code called `LLVMGetSymbolSize` for symbols without a
  containing section (for example file symbols), which can assert in debug
  builds.
- Fix:
  - Guard section iterator end after `LLVMMoveToContainingSection`.
  - For symbols with no containing section, print section as `"(null)"` and
    size as `0` without calling `LLVMGetSymbolSize`.
- Files:
  - Fix: `llvm-c/llvm-c-test/object.c`
  - Coverage: `llvm-c/llvm-c-test/inputs/object_symbols.test`

## Upstreaming Plan

1. Upstream llvm-c-test fixes that are API-call correctness issues
   (`object.c` section-iterator and symbol-size guard).
2. Keep regression lit tests for previously failing behaviors
   (`echo_constant_fp.ll`, `echo_bitcast.ll`, `object_symbols.test`).
3. Keep regression lit tests for historical failures and ensure they pass on
   both C and Python implementations (`poison.ll`).
