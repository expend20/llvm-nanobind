# Progress: Fix Python llvm-c-test Implementation

**Last Updated:** December 18, 2025
**Current Phase:** Phases 1-3 Complete

## Quick Summary

| Status | Count | Tests |
|--------|-------|-------|
| ✅ Passing | 21 | ARM/disassemble, X86/disassemble, add_named_metadata_operand, atomics, calc, callsite_attributes, di-type-get-name, empty, float_ops, freeze, function_attributes, functions, get-di-tag, globals, invalid-bitcode, invoke, is_a_value_as_metadata, memops, objectfile, replace_md_operand, set_metadata |
| ❌ Failing | 2 | debug_info_new_format, echo |

**Progress:** 21/23 tests passing (91.30%)

---

## Completed Phases

### Phase 1: ModuleID Fix ✅
**Completed:** December 18, 2025
**Tests Fixed:** 5 (atomics.ll, float_ops.ll, freeze.ll, invoke.ll, memops.ll)

**Changes:**
- `llvm_c_test/echo.py`: Changed module name to `"<stdin>"` after parsing to match C version

### Phase 2: Error Message Format + Diagnostic Handler ✅
**Completed:** December 18, 2025
**Tests Fixed:** 2 (invalid-bitcode.test, empty.ll)

**Changes:**
- `llvm_c_test/module_ops.py`: Added `_extract_error_message()` HACK to format error messages
- `llvm_c_test/diagnostic.py`: Implemented `test_diagnostic_handler()` using context diagnostics

### Phase 3: Lazy Loading Support ✅
**Completed:** December 18, 2025
**Tests Fixed:** 1 (functions.ll)

**Changes:**
- `src/llvm-nanobind.cpp`: Added `lazy` parameter to `parse_bitcode_from_bytes()`
- `llvm_c_test/module_ops.py`: Pass `lazy` parameter and set module name to `"<stdin>"`

---

## Remaining Failures (2 tests)

### echo.ll - Custom Syncscope Crash
**Root Cause:** The test uses `syncscope("agent")` which is a custom, context-specific syncscope. When cloning atomic instructions, the syncscope ID is copied directly but IDs are context-specific integers.

**Regression Test:** `tests/regressions/test_syncscope_crash.py`

**Status:** Blocked - requires C++ binding work or LLVM-C API to translate syncscope names between contexts.

### debug_info_new_format.ll - Metadata ID Mismatch
**Root Cause:** Python implementation creates 1 extra metadata node before the DISubprogram, resulting in `!dbg !45` instead of `!dbg !44`.

**Regression Test:** `tests/regressions/test_dibuilder_metadata.py`

**Status:** Needs investigation - compare `debuginfo.py` with `debuginfo.c` to find ordering difference.

---

## Test Commands

```bash
# Run all Python tests
uv run run_llvm_c_tests.py --use-python

# Run with verbose output
uv run run_llvm_c_tests.py --use-python -v

# Run individual regression tests
uv run python tests/regressions/test_module_id_stdin.py
uv run python tests/regressions/test_lazy_materializable.py
uv run python tests/regressions/test_error_message_format.py
uv run python tests/regressions/test_dibuilder_metadata.py
uv run python tests/regressions/test_syncscope_crash.py
```

---

## Key Files Modified

| File | Changes |
|------|---------|
| `llvm_c_test/echo.py` | Module name fix (`<stdin>`) |
| `llvm_c_test/module_ops.py` | Error message extraction, lazy loading, module name fix |
| `llvm_c_test/diagnostic.py` | Implemented diagnostic handler test |
| `src/llvm-nanobind.cpp` | Added `lazy` parameter to `parse_bitcode_from_bytes()` |

---

## Next Steps (Phases 4-5)

### Phase 4: DIBuilder Metadata Order
- Compare `llvm_c_test/debuginfo.py` with `llvm-c/llvm-c-test/debuginfo.c`
- Find the extra metadata creation causing ID offset
- **Effort:** Medium-High

### Phase 5: Syncscope Support
- May need to add `LLVMGetSyncScopeName` binding (if it exists in LLVM-C)
- Or implement name-based translation in Python
- **Effort:** High
