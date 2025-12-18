# Progress: Fix Python llvm-c-test Implementation

**Last Updated:** December 18, 2025
**Current Phase:** Analysis Complete - Ready for Implementation

## Quick Summary

| Status | Count | Tests |
|--------|-------|-------|
| ✅ Passing | 13 | ARM/disassemble, X86/disassemble, add_named_metadata_operand, calc, callsite_attributes, di-type-get-name, function_attributes, get-di-tag, globals, is_a_value_as_metadata, objectfile, replace_md_operand, set_metadata |
| ❌ Failing | 10 | atomics, debug_info_new_format, echo, empty, float_ops, freeze, functions, invalid-bitcode, invoke, memops |

**Progress:** 13/23 tests passing (56.52%)

---

## Regression Tests Created

All failing tests now have standalone regression tests in `tests/regressions/`:

| Test | Root Cause | Status |
|------|------------|--------|
| `test_module_id_stdin.py` | ModuleID `<bytes>` vs `<stdin>` | ✅ Created |
| `test_syncscope_crash.py` | Custom syncscope ID crash | ✅ Existing |
| `test_lazy_materializable.py` | Lazy loading not supported for bytes | ✅ Created |
| `test_error_message_format.py` | Error message wrapping | ✅ Created |
| `test_dibuilder_metadata.py` | Metadata ID order difference | ✅ Created |

---

## Failing Tests by Category

### Category 1: ModuleID Mismatch (6 tests) - Ready for Fix

- [ ] `atomics.ll`
- [ ] `empty.ll`
- [ ] `float_ops.ll`
- [ ] `freeze.ll`
- [ ] `invoke.ll`
- [ ] `memops.ll`

**Fix:** Add `mod.name = "<stdin>"` in `echo.py` after parsing from stdin.

### Category 2: Custom Syncscope Crash (1 test) - Needs C++ Work

- [ ] `echo.ll`

**Status:** Blocked - requires C++ binding to translate syncscope IDs between contexts.

### Category 3: Lazy Module (1 test) - Needs C++ Work

- [ ] `functions.ll`

**Status:** Blocked - requires `lazy` parameter in `parse_bitcode_from_bytes`.

### Category 4: Error Message Format (1 test) - Ready for Fix

- [ ] `invalid-bitcode.test`

**Fix:** Add HACK to extract clean error message in `module_ops.py`.

### Category 5: DIBuilder Metadata (1 test) - Needs Investigation

- [ ] `debug_info_new_format.ll`

**Status:** Python creates 1 extra metadata node. Need to compare with C source.

---

## Phase Implementation Status

| Phase | Description | Tests Fixed | Status |
|-------|-------------|-------------|--------|
| 1 | ModuleID fix | 6 | ⏳ Ready |
| 2 | Error message format | 1 | ⏳ Ready |
| 3 | Lazy module support | 1 | ⏸️ Needs C++ |
| 4 | DIBuilder metadata | 1 | ⏸️ Needs investigation |
| 5 | Syncscope support | 1 | ⏸️ Needs C++ |

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

# Compare C vs Python for a specific test
./llvm-bin llvm-as < llvm-c/llvm-c-test/inputs/atomics.ll | ./build/llvm-c-test --echo
./llvm-bin llvm-as < llvm-c/llvm-c-test/inputs/atomics.ll | uv run llvm-c-test --echo
```

---

## Next Steps

1. **Phase 1 (Easy Win):** Fix ModuleID by adding `mod.name = "<stdin>"` in echo.py
   - Expected: 6 tests fixed → 19/23 passing (82.6%)

2. **Phase 2 (Easy Win):** Fix error message format with extraction HACK
   - Expected: 1 test fixed → 20/23 passing (87.0%)

3. **Phase 3 (C++ Work):** Add lazy parameter to parse_bitcode_from_bytes
   - Requires modifying `src/llvm-nanobind.cpp`
   - Use `LLVMGetBitcodeModuleInContext2` for lazy loading

4. **Phase 4 (Investigation):** DIBuilder metadata order
   - Compare `llvm_c_test/debuginfo.py` with `llvm-c/llvm-c-test/debuginfo.c`
   - Find the extra metadata creation

5. **Phase 5 (Complex):** Syncscope translation
   - May need to add `LLVMGetSyncScopeName` binding (if it exists)
   - Or implement name-based translation in Python

---

## Key Files

| File | Purpose |
|------|---------|
| `llvm_c_test/echo.py` | Module cloning (Phase 1, 5) |
| `llvm_c_test/module_ops.py` | Module dump commands (Phase 2, 3) |
| `llvm_c_test/debuginfo.py` | DIBuilder test (Phase 4) |
| `src/llvm-nanobind.cpp` | C++ bindings (Phase 3, 5) |
