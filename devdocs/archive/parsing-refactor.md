# LLVM Module Parsing API Refactor - Summary

## Goal & Scope

Refactored the LLVM bitcode/IR parsing API to:
1. Hide `MemoryBuffer` as an internal implementation detail
2. Fix crashes in lazy-loaded modules (dangling buffer references)
3. Provide comprehensive diagnostics via `LLVMParseError`
4. Offer a Pythonic context manager-based API

## Problems Solved

| Problem | Root Cause | Solution |
|---------|------------|----------|
| Lazy module crashes | MemoryBuffer freed while Module still referenced it | Discovered LLVM handles buffer ownership internally for lazy loading |
| Diagnostic double-free | Thread-local diagnostic handler with incorrect lifetime | Per-context diagnostic storage with proper callback |
| No textual IR parsing | Only bitcode APIs exposed | Added `parse_ir()` method |
| Manual memory management | Users had to create/manage MemoryBuffer | All buffer handling moved to C++ wrapper layer |

## New Parsing API

Three methods on `Context`:

### `parse_bitcode_from_file(filename: str, lazy: bool = False) -> ModuleManager`

Parse LLVM bitcode from a file.

```python
with ctx.parse_bitcode_from_file("app.bc", lazy=True) as mod:
    main = mod.get_function("main")
```

- Files >16KB automatically use mmap
- `lazy=True`: Only parse module header; load function bodies on-demand
- `lazy=False`: Parse entire module immediately

### `parse_bitcode_from_bytes(data: bytes | bytearray) -> ModuleManager`

Parse LLVM bitcode from in-memory bytes.

```python
bitcode = sys.stdin.buffer.read()
with ctx.parse_bitcode_from_bytes(bitcode) as mod:
    process(mod)
```

- Always eager parsing (no lazy option - Python GC makes lazy unsafe)
- No reference to `data` retained after parsing

### `parse_ir(source: str) -> ModuleManager`

Parse textual LLVM IR from string.

```python
ir = "define i32 @main() { ret i32 0 }"
with ctx.parse_ir(ir) as mod:
    main = mod.get_function("main")
```

- Always eager (textual IR doesn't support lazy parsing)

## Critical Discovery: Memory Buffer Ownership

**Key insight:** LLVM handles buffer ownership internally - we don't need to track it ourselves.

```cpp
// For EAGER loading:
LLVMParseBitcodeInContext2(ctx, buf, &mod);  // LLVM consumes buffer contents
LLVMDisposeMemoryBuffer(buf);                 // We dispose buffer after success

// For LAZY loading:
LLVMGetBitcodeModuleInContext2(ctx, buf, &mod);  // LLVM takes buffer ownership
// DO NOT dispose - LLVM's Module stores buffer internally!

// On FAILURE (both cases):
LLVMDisposeMemoryBuffer(buf);  // We must dispose on failure
```

This eliminated the need for an `m_memory_buffer` member in `LLVMModuleWrapper` - a simpler design than originally planned.

## Diagnostics System

### Per-Context Storage

```cpp
struct LLVMContextWrapper {
    std::vector<Diagnostic> m_diagnostics;
    
    void diagnostic_handler(LLVMDiagnosticInfoRef info);
    std::vector<Diagnostic> get_diagnostics() const;
    void clear_diagnostics();
};
```

Handler installed automatically in context constructor:

```cpp
LLVMContextSetDiagnosticHandler(m_ref, callback, this);
```

### Diagnostic Structure

```cpp
struct Diagnostic {
    std::string severity;   // "error", "warning", "remark", "note"
    std::string message;
    std::optional<int> line;
    std::optional<int> column;
};
```

## Exception Hierarchy

```
LLVMException (base)
  └── LLVMParseError
        - stores std::vector<Diagnostic>
        - accessible via get_diagnostics() method
        - error message includes formatted diagnostics
```

Usage:

```python
try:
    with ctx.parse_ir("invalid syntax {") as mod:
        pass
except llvm.LLVMParseError as e:
    for diag in e.get_diagnostics():
        print(f"{diag.severity}: {diag.message}")
```

## Design Decisions

### No `parse_bitcode_from_stdin()`

**Rationale:**
- Redundant with `parse_bitcode_from_bytes(sys.stdin.buffer.read())`
- Less Pythonic - hides memory implications
- User has better control over stdin reading (progress, errors, timeouts)

### No MemoryBuffer Exposure

**Rationale:**
- Implementation detail users don't need
- Prevents error-prone manual lifetime management
- C++ wrapper handles all buffer lifecycle internally

### No Lazy Option for `parse_bitcode_from_bytes()`

**Rationale:**
- Python's GC could collect the `bytes` object while LLVM still references it
- Always copying data is the safe default
- Users wanting lazy loading should use file-based API

## Migration Patterns

### From stdin parsing

```python
# OLD (removed)
membuf = llvm.create_memory_buffer_with_stdin()
mod = llvm.parse_bitcode_in_context(ctx, membuf, lazy=False)

# NEW
bitcode = sys.stdin.buffer.read()
with ctx.parse_bitcode_from_bytes(bitcode) as mod:
    process(mod)
```

### From file parsing

```python
# OLD (removed)
membuf = llvm.create_memory_buffer_with_file("app.bc")
mod = llvm.parse_bitcode_in_context(ctx, membuf, lazy=True)

# NEW
with ctx.parse_bitcode_from_file("app.bc", lazy=True) as mod:
    process(mod)
```

### From textual IR (new capability)

```python
# NEW (not previously available)
ir_source = """
define i32 @add(i32 %a, i32 %b) {
    %sum = add i32 %a, %b
    ret i32 %sum
}
"""
with ctx.parse_ir(ir_source) as mod:
    add_fn = mod.get_function("add")
```

## Removed APIs

| API | Replacement |
|-----|-------------|
| `create_memory_buffer_with_stdin()` | `sys.stdin.buffer.read()` + `parse_bitcode_from_bytes()` |
| `parse_bitcode_in_context()` | `Context.parse_bitcode_from_file()` or `Context.parse_bitcode_from_bytes()` |
| `LLVMMemoryBufferWrapper` (Python binding) | Internal only, not exposed |
| `context_set_diagnostic_handler()` | Automatic - handler installed on context creation |
| Thread-local diagnostic globals | Per-context `m_diagnostics` vector |

## API Summary Table

| Method | Input | Lazy? | Buffer Handling |
|--------|-------|-------|-----------------|
| `parse_bitcode_from_file()` | File path | Optional | Internal (LLVM owns if lazy) |
| `parse_bitcode_from_bytes()` | bytes/bytearray | No | Internal (copied, disposed after) |
| `parse_ir()` | str | No | Internal (disposed after) |

## Lifetime Integration

All parsed modules integrate with the validity token system:

```
Context Token
    └── Module Token (invalidated on module.__exit__())
            ├── Function (checks module token)
            │       └── BasicBlock (checks module token)
            │               └── Instruction (checks module token)
            └── GlobalVariable (checks module token)
```

Accessing any child after module disposal raises `LLVMError`.
