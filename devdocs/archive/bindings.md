# LLVM-nanobind Bindings: Completion Summary

## Goal and Scope

This task created Pythonic bindings for the LLVM-C API using nanobind. The goal was to expose LLVM's intermediate representation (IR) capabilities to Python in a way that:
1. Feels natural to Python developers (object methods instead of C-style factory functions)
2. Provides safe lifetime management (prevent use-after-free and double-free errors)
3. Supports deterministic resource cleanup via context managers
4. Raises clear Python exceptions instead of crashing

## Key Architectural Decisions

### 1. Hybrid Ownership Model

The bindings use a combination of three mechanisms:
- **Context managers** for explicit lifetime control of major resources (Context, Module, Builder)
- **Validity tokens** (`shared_ptr<ValidityToken>`) to detect use-after-parent-destruction
- **Reference counting** via nanobind to prevent premature garbage collection

### 2. Flat Token Model (Current Implementation)

All wrapper objects track only the **Context token**. When the context is destroyed, all objects become invalid. This simplifies implementation while still providing safety guarantees.

```
Context Token (invalidated on context destruction)
    ├── Module
    ├── Function
    ├── BasicBlock
    ├── Value/Instruction
    ├── Type
    └── Builder
```

A hierarchical token model (module → function → block → instruction) was designed but not implemented. The flat model proved sufficient for core use cases.

### 3. Exception Hierarchy

Three-tier exception system based on error severity:

| Exception | Base | Purpose |
|-----------|------|---------|
| `LLVMError` | `Exception` | Recoverable runtime errors (I/O, parsing) |
| `LLVMAssertionError` | `AssertionError` | Programming mistakes (type mismatches, invalid indices) |
| `LLVMMemoryError` | `SystemExit` | Lifetime violations - **not catchable with `except Exception`** |

The `LLVMMemoryError` design intentionally prevents accidental continuation after memory safety violations.

### 4. Wrapper Structure

All wrappers follow this pattern:
```cpp
struct LLVMXxxWrapper : NoMoveCopy {
    LLVMXxxRef m_ref = nullptr;
    std::shared_ptr<ValidityToken> m_context_token;
    
    void check_valid() const {
        if (!m_ref) throw LLVMMemoryError("Object is null");
        if (!m_context_token || !m_context_token->is_valid())
            throw LLVMMemoryError("Object used after context was destroyed");
    }
    // All methods call check_valid() first
};
```

## Established Patterns and Conventions

### API Design Patterns

1. **Context managers for resource ownership:**
   ```python
   with llvm.create_context() as ctx:
       with ctx.create_module("example") as mod:
           # Safe usage
   ```

2. **Factory methods on owning objects:**
   - Types created via `ctx.int32_type()`, `ctx.function_type()`, etc.
   - Modules created via `ctx.create_module()`
   - Functions created via `mod.add_function()`
   - BasicBlocks created via `func.append_basic_block()`

3. **Builder pattern for instructions:**
   ```python
   with ctx.create_builder() as builder:
       builder.position_at_end(entry_block)
       result = builder.add(lhs, rhs, name="sum")
   ```

### Naming Conventions

- **C++**: `LLVMXxxWrapper` for wrapper structs, `m_` prefix for members, `snake_case` for methods
- **Python**: Follow PEP 8, `snake_case` for methods and properties
- **Enums**: Exposed as Python enums (e.g., `llvm.Linkage.External`, `llvm.IntPredicate.EQ`)

### Testing Pattern (Golden Master)

C++ tests serve as the "golden master" - they output LLVM IR to stdout, which is saved and compared against Python test output:

```
C++ test (golden master) → tests/output/*.ll → diff ← Python test
```

Requirements:
- Output must be deterministic (no timestamps, PIDs, addresses)
- Running `./build/test_foo` directly must match stored output exactly
- Each test outputs valid LLVM IR with diagnostic comments

## Technical Insights and Gotchas

### LLVM Lifetime Requirements

LLVM has strict parent-owns-children semantics:
- Children must be freed before parents
- Python's GC provides no destructor ordering guarantees
- The validity token pattern solves this by invalidating children when parents are destroyed

### Module Cloning

`mod.clone()` returns a **module manager**, not a module directly. This maintains consistent ownership semantics:
```python
with mod.clone() as cloned:
    # Use cloned module
```

### Builder Independence

Builders have independent lifetime from the blocks they operate on:
- A builder can outlive blocks it was positioned in
- Instructions remain valid after the builder that created them is disposed
- Builders only validate they have a valid context, not the blocks they reference

### Not Implemented (By Design)

Several features from the aspirational design were intentionally not implemented:
- Hierarchical token checking (module → function → block → instruction)
- `detach()` / `insert_into()` for instructions/blocks
- `erase()` for basic blocks and instructions
- Module-level token invalidation

The flat context-token model proved sufficient and simpler.

## Implemented API Summary

### Core Objects
- **Context**: Type factory, module/builder creation, `discard_value_names` property
- **Module**: Name, source filename, data layout, target triple, function/global management, verification, cloning
- **Function**: Parameters, linkage, calling convention, basic block management, erase
- **BasicBlock**: Name, navigation (next/prev), instruction access (first/last/terminator)
- **Builder**: Positioning, all arithmetic/bitwise/memory/comparison/cast/control-flow operations
- **Value**: Type, name, string representation, constant predicates
- **Type**: Kind, string representation, type predicates, `int_width` for integers

### Constants
`const_int`, `const_real`, `const_null`, `const_all_ones`, `undef`, `poison`, `const_array`, `const_struct`, `const_vector`

### Enumerations
`Linkage`, `Visibility`, `CallConv`, `IntPredicate`, `RealPredicate`, `TypeKind`

### Test Coverage
15 test categories covering context, module, types, function, basic blocks, arithmetic, memory, control flow, casts, comparisons, constants, globals, phi nodes, and integration tests (factorial, struct manipulation).

## Related Documentation

- `devdocs/memory-model.md` - Detailed lifetime management design and future hierarchical model
- `devdocs/DEBUGGING.md` - Guidelines for debugging crashes and adding safety checks
- `AGENTS.md` - Build commands, test commands, and development workflow
