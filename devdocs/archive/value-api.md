# Value API Refactor (Archived)

Completed: December 2024

## Summary

Moved value inspection functions from module scope to Value class methods and properties, following the Pythonic API design philosophy.

## What Changed

### Global Functions Removed → Value Properties/Methods

| Old (Global) | New (Value) | Type |
|--------------|-------------|------|
| `llvm.value_is_null(val)` | `val.is_null` | property |
| `llvm.const_int_get_zext_value(val)` | `val.const_zext_value` | property |
| `llvm.const_int_get_sext_value(val)` | `val.const_sext_value` | property |
| `llvm.const_bitcast(val, ty)` | `val.const_bitcast(ty)` | method |
| `llvm.delete_instruction(inst)` | `inst.delete_instruction()` | method |
| `llvm.is_a_value_as_metadata(val)` | `val.is_value_as_metadata` | property |
| `llvm.value_as_metadata(val)` | `val.as_metadata()` | method |

### All is_a_* Methods Changed to Properties

Changed from method calls to properties for consistency:
- `val.is_a_constant_int()` → `val.is_a_constant_int`
- `val.is_a_function()` → `val.is_a_function`
- `val.is_declaration()` → `val.is_declaration`
- etc.

## Key Decisions

1. **`const_zext_value` / `const_sext_value` naming**: Added `const_` prefix to clarify these only work on constant integers and will throw if used on other value types.

2. **`delete_instruction()` naming**: Kept explicit name rather than just `delete()` to avoid confusion with `delete_global()` which already exists.

3. **Properties vs methods for predicates**: All `is_a_*` and `is_*` predicates are now properties, following Python convention where simple boolean checks don't need parentheses.

4. **Forward declarations for cross-type methods**: `as_metadata()` returns `LLVMMetadataWrapper`, so it's declared in the struct but implemented after `LLVMMetadataWrapper` is defined.

## Implementation Pattern

For methods that return types defined later in the file:

```cpp
// In LLVMValueWrapper struct - declaration only
LLVMMetadataWrapper as_metadata() const;

// After LLVMMetadataWrapper is defined - implementation
inline LLVMMetadataWrapper LLVMValueWrapper::as_metadata() const {
  check_valid();
  return LLVMMetadataWrapper(LLVMValueAsMetadata(m_ref), m_context_token);
}
```

## Files Modified

- `src/llvm-nanobind.cpp` - C++ implementation and bindings
- `tests/test_constants.py`, `tests/test_globals.py` - Test updates
- `llvm_c_test/echo.py` - Major updates for all is_a_* and const_bitcast
- `llvm_c_test/metadata.py` - delete_instruction, is_value_as_metadata
- `llvm_c_test/debuginfo.py` - as_metadata
- `llvm_c_test/module_ops.py`, `llvm_c_test/attributes.py` - is_declaration, is_a_call_inst

## Verification

All tests passing:
- `uv run run_tests.py` - 15/15 C++ tests, 15/15 Python tests
- `uv run run_llvm_c_tests.py --use-python` - 34/34 lit tests
- `uvx ty check` - Type checking passes
