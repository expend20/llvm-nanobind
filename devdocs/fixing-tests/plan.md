# Plan: Fix Python llvm-c-test Implementation

## Overview

When running lit tests with `--use-python`, 10 out of 23 tests fail. This plan documents the root causes and fixes for each category of failure.

## Current Test Status (December 18, 2025)

```
Passed: 21 (91.30%)
Failed:  2 (8.70%)
```

## Root Cause Analysis

### Category 1: ModuleID Mismatch (6 tests)

**Tests Affected:**
- `atomics.ll`
- `empty.ll`
- `float_ops.ll`
- `freeze.ll`
- `invoke.ll`
- `memops.ll`

**Root Cause:**
When parsing bitcode from bytes, the module ID is set to `'<bytes>'` but the LLVM C test expects `'<stdin>'`. This is a regression from the rewrite that replaced stdin-specific parsing with `parse_bitcode_from_bytes`.

**Regression Test:** `tests/regressions/test_module_id_stdin.py`

**Fix:**
In `echo.py`, rename the module to `'<stdin>'` after parsing when reading from stdin. This is acceptable since llvm-c-test is just for LLVM comparison.

```python
# In echo.py, after parsing:
mod.name = "<stdin>"
```

---

### Category 2: Custom Syncscope Crash (1 test)

**Tests Affected:**
- `echo.ll`

**Root Cause:**
The test uses `syncscope("agent")` which is a custom, context-specific syncscope. When echo.py clones atomic instructions, it copies the syncscope ID directly from source to destination. However, syncscope IDs are context-specific integers that differ between contexts.

The crash occurs in `LLVMPrintModuleToString` when LLVM tries to look up the syncscope name for an invalid ID.

**Regression Test:** `tests/regressions/test_syncscope_crash.py` (existing)

**Fix Options:**
1. Add C++ binding to get syncscope name from ID (requires C++ API access)
2. Translate syncscope IDs by maintaining a mapping during cloning
3. Skip custom syncscopes in echo.py (document as limitation)

---

### Category 3: Lazy Module Missing "Materializable" Comment (1 test)

**Tests Affected:**
- `functions.ll` (`--lazy-module-dump` command)

**Root Cause:**
Two issues:
1. `module_ops.py` doesn't pass `lazy=True` to the parse function
2. `parse_bitcode_from_bytes` doesn't have a `lazy` parameter (only `parse_bitcode_from_file` does)

When truly lazy-loaded, LLVM's `PrintModuleToString` automatically includes `; Materializable` comments before function definitions with empty bodies `{}`.

**Regression Test:** `tests/regressions/test_lazy_materializable.py`

**Fix:**
1. Add `lazy` parameter to `parse_bitcode_from_bytes` C++ binding (use `LLVMGetBitcodeModuleInContext2` instead of `LLVMParseBitcodeInContext2`)
2. Update `module_ops.py` to use `lazy=True` for `--lazy-module-dump`

---

### Category 4: Error Message Format (1 test)

**Tests Affected:**
- `invalid-bitcode.test`

**Root Cause:**
The Python exception wrapper adds extra text to error messages.

**Expected (C):**
```
Error parsing bitcode: Unknown attribute kind (255)
```

**Actual (Python):**
```
Error parsing bitcode: Failed to parse LLVM IR:
  error: Unknown attribute kind (255)
```

**Regression Test:** `tests/regressions/test_error_message_format.py`

**Fix:**
HACK in `module_ops.py` to extract the clean error message:
```python
def extract_error_message(exception_msg: str) -> str:
    """Extract core error message from LLVMParseError format."""
    for line in exception_msg.strip().split("\n"):
        line = line.strip()
        if line.startswith("error:"):
            return line[len("error:"):].strip()
    return exception_msg
```

---

### Category 5: DIBuilder Metadata ID Mismatch (1 test)

**Tests Affected:**
- `debug_info_new_format.ll`

**Root Cause:**
The Python implementation creates 1 extra metadata node before the DISubprogram, resulting in `!dbg !45` instead of `!dbg !44`.

Analysis shows:
- Python: 54 metadata nodes before DISubprogram
- C: 53 metadata nodes before DISubprogram

This indicates the order of metadata creation differs between implementations.

**Regression Test:** `tests/regressions/test_dibuilder_metadata.py`

**Fix:**
Compare `llvm_c_test/debuginfo.py` with `llvm-c/llvm-c-test/debuginfo.c` to find where the extra metadata is being created or where order differs.

---

## Implementation Phases

### Phase 1: ModuleID Fix ✅ COMPLETE
**Priority:** High (fixes 6 tests)
**Effort:** Low
**File:** `llvm_c_test/echo.py`
**Result:** Fixed 5 tests (atomics, float_ops, freeze, invoke, memops)

### Phase 2: Error Message Format Fix ✅ COMPLETE
**Priority:** Medium (fixes 1 test)
**Effort:** Low
**Files:** `llvm_c_test/module_ops.py`, `llvm_c_test/diagnostic.py`
**Result:** Fixed 2 tests (invalid-bitcode, empty)

### Phase 3: Lazy Module Support ✅ COMPLETE
**Priority:** Medium (fixes 1 test)
**Effort:** Medium
**Files:** `src/llvm-nanobind.cpp`, `llvm_c_test/module_ops.py`
**Result:** Fixed 1 test (functions)

### Phase 4: DIBuilder Metadata Order
**Priority:** Medium (fixes 1 test)
**Effort:** Medium-High (requires careful comparison)
**File:** `llvm_c_test/debuginfo.py`
**Status:** Pending

### Phase 5: Syncscope Support
**Priority:** Low (fixes 1 test, complex)
**Effort:** High (may need C++ binding work)
**Files:** `src/llvm-nanobind.cpp`, `llvm_c_test/echo.py`
**Status:** Pending

---

## Summary Table

| Root Cause | Tests | Regression Test | Fix Effort | Phase | Status |
|------------|-------|-----------------|------------|-------|--------|
| ModuleID `<bytes>` vs `<stdin>` | 5 | `test_module_id_stdin.py` | Low | 1 | ✅ Done |
| Error message format | 2 | `test_error_message_format.py` | Low | 2 | ✅ Done |
| Lazy module support | 1 | `test_lazy_materializable.py` | Medium | 3 | ✅ Done |
| DIBuilder metadata order | 1 | `test_dibuilder_metadata.py` | Medium-High | 4 | Pending |
| Custom syncscope crash | 1 | `test_syncscope_crash.py` | High | 5 | Pending |
| **Total Fixed** | **8** | | | | |
| **Remaining** | **2** | | | | |

---

## Success Criteria

All 23 lit tests pass with `--use-python`:
```bash
uv run run_llvm_c_tests.py --use-python
# Expected: 23 tests passed, 0 failed
```
