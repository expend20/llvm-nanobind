# Plan: Fix Python llvm-c-test Implementation

## Overview

All 23 lit tests now pass with `--use-python`. This document records the issues found and fixes applied.

## Final Test Status (December 18, 2025)

```
Passed: 23 (100.00%)
Failed:  0 (0.00%)
```

## Issues Fixed

### Issue 1: echo.ll - Syncscope Crash ✅ FIXED

**Root Cause:** The `get_module_context` binding was broken. It ignored the actual context returned by `LLVMGetModuleContext` and always returned a wrapper around the global context.

**Fix Applied:**
1. Added "borrowed" context wrapper support - non-owning wrappers that share the validity token with the owning context
2. Fixed `get_module_context` to return a borrowed wrapper around the module's actual context
3. Refactored diagnostic handler to use a global registry (thread-safe map) instead of per-wrapper storage

**Files Changed:**
- `src/llvm-nanobind.cpp`: Added `m_borrowed` flag, `DiagnosticRegistry`, fixed `get_module_context`

---

### Issue 2: debug_info_new_format.ll - Metadata ID Mismatch ✅ FIXED

**Root Causes:**
1. `dibuilder_create_struct_type` binding was missing parameters (`elements`, `runtime_lang`, `unique_identifier`)
2. `debuginfo.py` passed `None` instead of `foo_var1` for `associated` parameter in `dibuilder_create_dynamic_array_type`
3. C test code had bugs (we fixed them, diverging from LLVM upstream)

**Fix Applied:**
1. Extended `dibuilder_create_struct_type` binding with all C API parameters
2. Updated `debuginfo.py` to pass `foo_var1` and use extended struct type binding
3. Fixed C test bugs and updated lit test expected output

**Files Changed:**
- `src/llvm-nanobind.cpp`: Extended `dibuilder_create_struct_type`
- `llvm_c_test/debuginfo.py`: Pass correct parameters
- `llvm-c/llvm-c-test/debuginfo.c`: Fixed name/length bugs (diverges from upstream)
- `llvm-c/llvm-c-test/inputs/debug_info_new_format.ll`: Updated expected output

---

### Earlier Issues (Phases 1-3)

| Issue | Tests Fixed | Fix |
|-------|-------------|-----|
| ModuleID `<bytes>` vs `<stdin>` | 5 | Set module name to `"<stdin>"` after parsing |
| Error message format | 2 | Extract clean error from exception |
| Lazy module support | 1 | Add `lazy` parameter to `parse_bitcode_from_bytes` |

---

## Implementation Phases

### Phase 1: ModuleID Fix ✅ COMPLETE
### Phase 2: Error Message Format Fix ✅ COMPLETE  
### Phase 3: Lazy Module Support ✅ COMPLETE
### Phase 4: Fix `get_module_context` binding ✅ COMPLETE
### Phase 5: Fix DIBuilder metadata order ✅ COMPLETE

---

## LLVM Upstream Bug Fixes

The following bugs were fixed in our vendored C test code. These should be submitted as PRs to LLVM upstream:

1. **Enumerator name bug** in `debuginfo.c` line 209:
   - Original: `LLVMDIBuilderCreateEnumerator(DIB, "Test_B", strlen("Test_C"), 2, true)`
   - Fixed: `LLVMDIBuilderCreateEnumerator(DIB, "Test_C", strlen("Test_C"), 2, true)`

2. **Forward decl name length bug** in `debuginfo.c` line 274:
   - Original: `"Class1", 5` (produces "Class")
   - Fixed: `"Class1", 6` (produces "Class1")

---

## Success Criteria ✅ MET

All 23 lit tests pass with `--use-python`:
```bash
uv run run_llvm_c_tests.py --use-python
# Result: 23 tests passed, 0 failed
```
