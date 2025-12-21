# DIBuilder API Refactor (Completed)

Moved 40+ `llvm.dibuilder_*` global functions to `DIBuilder` class methods and added context manager support.

## Key Learnings

### 1. Context Manager Pattern for Resource Cleanup

DIBuilder now uses the same `with` statement pattern as `IRBuilder`:

```python
# New pattern
with llvm.create_dibuilder(mod) as dib:
    file = dib.create_file("foo.c", ".")
    # ...
    dib.finalize()
```

This requires a Manager class (`LLVMDIBuilderManager`) that:
- Returns the actual `DIBuilder` from `__enter__`
- Handles cleanup in `__exit__`
- Uses `nb::rv_policy::reference_internal` for the `__enter__` return value

### 2. nanobind `__exit__` Signature

The `__exit__` method needs `.none()` annotations on all three arguments for Python's context manager protocol to work:

```cpp
// Correct
.def("__exit__", &Manager::exit, "exc_type"_a.none(),
     "exc_value"_a.none(), "traceback"_a.none())

// Wrong - causes TypeError at runtime
.def("__exit__", &Manager::exit)
```

### 3. Functions That Stay Global

Some debug info functions take `Context` instead of `DIBuilder`, so they remain as module-level functions:

- `llvm.dibuilder_create_debug_location(ctx, ...)` - takes Context
- `llvm.metadata_as_value(ctx, ...)` - takes Context
- `llvm.set_subprogram(func, subprogram)` - utility
- `llvm.metadata_replace_all_uses_with(...)` - utility
- `llvm.di_subprogram_replace_type(...)` - utility

### 4. Method Implementation Pattern

All methods follow the same pattern - the `dib` argument is removed since it's now `this`:

```cpp
// Method declaration in class
LLVMMetadataWrapper create_file(const std::string& filename,
                                const std::string& directory);

// Implementation
LLVMMetadataWrapper LLVMDIBuilderWrapper::create_file(
    const std::string& filename, const std::string& directory) {
  check_valid();
  return LLVMMetadataWrapper(
      LLVMDIBuilderCreateFile(m_ref, filename.c_str(), filename.size(),
                              directory.c_str(), directory.size()),
      m_module_token);
}
```

## API Changes Summary

| Old API | New API |
|---------|---------|
| `llvm.create_dibuilder(mod)` | `with llvm.create_dibuilder(mod) as dib:` |
| `llvm.dibuilder_create_file(dib, f, d)` | `dib.create_file(f, d)` |
| `llvm.dibuilder_create_compile_unit(dib, ...)` | `dib.create_compile_unit(...)` |
| `llvm.replace_arrays(dib, cts, arrs)` | `dib.replace_arrays(cts, arrs)` |
| ... (40+ functions total) | |

## Files Modified

1. **`src/llvm-nanobind.cpp`**:
   - Added method declarations to `LLVMDIBuilderWrapper` (~lines 5350-5637)
   - Added method implementations (~lines 5677-6070)
   - Added `LLVMDIBuilderManager` class
   - Updated Python bindings for `DIBuilder` class
   - Removed old global `dibuilder_*` functions

2. **`llvm_c_test/debuginfo.py`**:
   - Updated all calls to use new method API
   - Wrapped DIBuilder creation in `with` statement
