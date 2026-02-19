# Known LLVM-C API Bugs and Limitations

Last updated: February 19, 2026

This document tracks issues relevant to llvm-c-test parity work.

## 1. Poison Constant ValueKind Mismatch

- Status: Reproduced in both C and Python implementations.
- Scope: `--echo`
- Symptom: `LLVMGetValueKind` reports poison as the wrong kind during cloning.
- Error text: `LLVMGetValueKind returned incorrect type`
- Lit coverage: `llvm-c/llvm-c-test/inputs/echo_poison_known_bug.ll`
- Notes: This currently appears to be an upstream LLVM-C API behavior issue.

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

## 5. Object Symbol Section Name Crash in C llvm-c-test

- Status: Fixed in vendored `llvm-c-test` code.
- Scope: `--object-list-symbols`
- Root cause: C test code called `LLVMGetSectionName` after
  `LLVMMoveToContainingSection` without checking section-iterator end.
- Fix: Guard with `LLVMObjectFileIsSectionIteratorAtEnd` and print `"(null)"`
  when no containing section exists.
- Files:
  - Fix: `llvm-c/llvm-c-test/object.c`
  - Coverage: `llvm-c/llvm-c-test/inputs/object_symbols.test`

## Upstreaming Plan

1. Upstream llvm-c-test fixes that are API-call correctness issues
   (`object.c` section-iterator end guard).
2. Keep regression lit tests for previously failing behaviors
   (`echo_constant_fp.ll`, `echo_bitcast.ll`, `object_symbols.test`).
3. For unresolved upstream API behavior (`echo_poison_known_bug.ll`), keep a
   negative test and reference it in upstream discussion.
