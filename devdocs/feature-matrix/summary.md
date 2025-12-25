# Feature Matrix Summary

Comprehensive tracking of LLVM-C API implementation in llvm-nanobind Python bindings.

**Last Updated:** 2024-12-25

---

## Quick Reference

```python
import llvm

# Create context and module
with llvm.create_context() as ctx:
    with ctx.create_module("example") as mod:
        # Types
        i32 = ctx.types.i32
        fn_ty = ctx.types.function(i32, [i32, i32])
        
        # Function and basic block
        fn = mod.add_function("add", fn_ty)
        bb = fn.append_basic_block("entry")
        
        # Build instructions
        with ctx.create_builder(bb) as b:
            result = b.add(fn.get_param(0), fn.get_param(1))
            b.ret(result)
        
        # Output
        print(mod)
```

---

## Coverage Summary

| Header | Total | ✅ Impl | 🚫 Skip | ❌ TODO | Coverage |
|--------|-------|---------|---------|---------|----------|
| **Core.h** | 640 | 413 | 44 | 183 | 64.5% |
| **DebugInfo.h** | 99 | ~50 | 0 | ~49 | ~50% |
| **Target.h** | 22 | 0 | 0 | 22 | 0% |
| **TargetMachine.h** | 29 | 7 | 0 | 22 | 24% |
| **Object.h** | 31 | 23 | 0 | 8 | 74% |
| **Analysis.h** | 4 | 1 | 0 | 3 | 25% |
| **BitReader.h** | 8 | 3 | 4 | 1 | 37.5% |
| **BitWriter.h** | 4 | 0 | 0 | 4 | 0% |
| **IRReader.h** | 1 | 1 | 0 | 0 | 100% |
| **PassBuilder.h** | 15 | 0 | 0 | 15 | 0% |
| **Disassembler.h** | 6 | 3 | 0 | 3 | 50% |
| **Linker.h** | 1 | 0 | 0 | 1 | 0% |
| **Error.h** | 7 | 0 | 7 | 0 | 0%* |
| **Other** | 12 | 0 | 0 | 12 | 0% |
| **Total** | **~880** | **~501** | **~55** | **~324** | **~57%** |

*Error.h uses Python exceptions instead of C-style error handling.

---

## Implementation Status by Category

### ✅ Well Covered (>70%)

| Feature | Coverage | Notes |
|---------|----------|-------|
| Module creation/properties | 95% | Full CRUD support |
| Type system | 90% | All common types |
| Basic block operations | 85% | Iteration, manipulation |
| Builder - common instructions | 80% | Arithmetic, memory, control flow |
| Object file reading | 74% | Sections, symbols, relocations |
| Global variables | 80% | Properties, initializers |
| Functions | 75% | Parameters, attributes |

### ⚠️ Partial Coverage (30-70%)

| Feature | Coverage | Notes |
|---------|----------|-------|
| Debug info | ~50% | Core DIBuilder, needs more types |
| Disassembler | 50% | Basic disassembly works |
| Bitcode reading | 37.5% | Context versions only |
| Metadata | 40% | Basic creation and attachment |
| Attributes | 40% | Enum attributes, needs string/type |

### ❌ Not Implemented (<30%)

| Feature | Coverage | Priority | Notes |
|---------|----------|----------|-------|
| BitWriter | 0% | **High** | Can't save bitcode |
| PassBuilder | 0% | **High** | Can't optimize |
| TargetMachine | 24% | **High** | Can't generate object code |
| Target Data | 0% | Medium | Type layout queries |
| Linker | 0% | Medium | Module linking |
| Comdat | 0% | Low | Windows/COFF support |

---

## Priority Implementation Gaps

### 🔴 High Priority - Blocking Core Workflows

1. **Bitcode Writing** (`BitWriter.h`)
   ```python
   # Needed:
   mod.write_bitcode_to_file("output.bc")
   bc_bytes = mod.write_bitcode_to_bytes()
   ```

2. **Optimization Passes** (`PassBuilder.h`)
   ```python
   # Needed:
   llvm.run_passes(mod, "default<O2>", target_machine)
   ```

3. **Code Generation** (`TargetMachine.h`)
   ```python
   # Needed:
   tm = target.create_target_machine(cpu="generic")
   obj_bytes = tm.emit_to_memory_buffer(mod, llvm.ObjectFile)
   ```

4. **Host Queries** (`Target.h`)
   ```python
   # Needed:
   triple = llvm.get_default_target_triple()
   cpu = llvm.get_host_cpu_name()
   ```

### 🟡 Medium Priority - Enhanced Functionality

5. **Module Linking** (`Linker.h`)
   ```python
   # Needed:
   mod.link(other_mod)
   ```

6. **Function Verification** (`Analysis.h`)
   ```python
   # Needed:
   if not fn.verify():
       print(fn.get_verification_error())
   ```

7. **Target Data Queries** (`Target.h`)
   ```python
   # Needed:
   size = target_data.abi_size_of_type(struct_ty)
   ```

---

## Intentionally Skipped (🚫)

### Global Context APIs
All functions using `LLVMGetGlobalContext()` - safety risk.

### Legacy Pass Manager  
`LLVMCreatePassManager`, `LLVMRunPassManager`, etc. - use PassBuilder.

### Deprecated Functions
`LLVMBuildLoad`, `LLVMBuildGEP`, `LLVMBuildCall`, etc. - use `*2` versions.

### Unsafe for Embedding
`LLVMShutdown` - would corrupt Python process.

### Internal APIs
`LLVMCreateMessage`, `LLVMDisposeMessage` - internal memory management.

---

## Detailed Matrix Files

| File | Contents | Size |
|------|----------|------|
| [core.md](core.md) | Core.h - All 640 functions with Python API | 49KB |
| [debuginfo.md](debuginfo.md) | DebugInfo.h - 99 functions with examples | 13KB |
| [target.md](target.md) | Target/TargetMachine - 51 functions | 8KB |
| [misc.md](misc.md) | All other headers - 89 functions | 11KB |

---

## API Design Patterns

### Context Managers
```python
with llvm.create_context() as ctx:
    with ctx.create_module("mod") as mod:
        with ctx.create_builder(bb) as b:
            # Resources automatically cleaned up
```

### Properties for Get/Set
```python
mod.name = "my_module"        # LLVMSetModuleIdentifier
print(mod.name)               # LLVMGetModuleIdentifier
```

### Pythonic Iteration
```python
for fn in mod.functions:       # LLVMGetFirst/NextFunction
    for bb in fn.basic_blocks: # LLVMGetFirst/NextBasicBlock
        for inst in bb.instructions:
            print(inst)
```

### Safety Checks
```python
# Raises LLVMMemoryError instead of crashing
try:
    dead_module.functions  # Module was disposed
except llvm.LLVMMemoryError:
    print("Module no longer valid")
```

### Rich Exceptions
```python
try:
    mod = ctx.parse_ir("invalid")
except llvm.LLVMParseError as e:
    for diag in ctx.get_diagnostics():
        print(f"{diag.line}:{diag.column}: {diag.message}")
```
