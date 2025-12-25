# Target.h + TargetMachine.h Feature Matrix

LLVM-C Target and Code Generation API implementation status with Python API mappings.

## Legend

| Status | Meaning |
|--------|---------|
| ✅ | Implemented |
| ❌ | Not implemented |

---

## Quick Start Example

```python
import llvm

# Initialize targets (must call before using target APIs)
llvm.initialize_all_targets()
llvm.initialize_all_target_mcs()
llvm.initialize_all_asm_printers()
llvm.initialize_all_asm_parsers()

# List available targets
for target in llvm.targets():
    print(f"{target.name}: {target.description}")
    print(f"  Has JIT: {target.has_jit}")
    print(f"  Has Target Machine: {target.has_target_machine}")
```

---

## Target Initialization

| C API | Python API | Example |
|-------|------------|---------|
| `LLVMInitializeAllTargetInfos` | `llvm.initialize_all_target_infos()` | - |
| `LLVMInitializeAllTargets` | `llvm.initialize_all_targets()` | `llvm.initialize_all_targets()` |
| `LLVMInitializeAllTargetMCs` | `llvm.initialize_all_target_mcs()` | `llvm.initialize_all_target_mcs()` |
| `LLVMInitializeAllAsmPrinters` | `llvm.initialize_all_asm_printers()` | `llvm.initialize_all_asm_printers()` |
| `LLVMInitializeAllAsmParsers` | `llvm.initialize_all_asm_parsers()` | `llvm.initialize_all_asm_parsers()` |
| `LLVMInitializeAllDisassemblers` | `llvm.initialize_all_disassemblers()` | `llvm.initialize_all_disassemblers()` |
| `LLVMInitializeNativeTarget` | `llvm.initialize_native_target()` | `llvm.initialize_native_target()` |
| `LLVMInitializeNativeAsmPrinter` | `llvm.initialize_native_asm_printer()` | - |
| `LLVMInitializeNativeAsmParser` | `llvm.initialize_native_asm_parser()` | - |
| `LLVMInitializeNativeDisassembler` | `llvm.initialize_native_disassembler()` | - |

```python
# Initialize for current platform
llvm.initialize_native_target()
llvm.initialize_native_asm_printer()

# Or initialize everything
llvm.initialize_all_targets()
llvm.initialize_all_target_mcs()
llvm.initialize_all_asm_printers()
```

---

## Target Queries

| C API | Python API | Example |
|-------|------------|---------|
| `LLVMGetFirstTarget` | `llvm.get_first_target()` | `target = llvm.get_first_target()` |
| `LLVMGetNextTarget` | `target.next_target` | `target = target.next_target` |
| `LLVMGetTargetFromName` | ❌ | TODO |
| `LLVMGetTargetFromTriple` | ❌ | TODO |
| `LLVMGetTargetName` | `target.name` | `name = target.name` |
| `LLVMGetTargetDescription` | `target.description` | `desc = target.description` |
| `LLVMTargetHasJIT` | `target.has_jit` | `if target.has_jit:` |
| `LLVMTargetHasTargetMachine` | `target.has_target_machine` | `if target.has_target_machine:` |
| `LLVMTargetHasAsmBackend` | `target.has_asm_backend` | `if target.has_asm_backend:` |

```python
# Iterate all targets
def all_targets():
    target = llvm.get_first_target()
    while target:
        yield target
        target = target.next_target

for target in all_targets():
    print(f"{target.name}: {target.description}")

# Or use the convenience iterator
for target in llvm.targets():
    print(target.name)
```

---

## Target Machine (Not Fully Implemented)

| C API | Python API | Status |
|-------|------------|--------|
| `LLVMCreateTargetMachineOptions` | ❌ | TODO |
| `LLVMDisposeTargetMachineOptions` | ❌ | TODO |
| `LLVMTargetMachineOptionsSetCPU` | ❌ | TODO |
| `LLVMTargetMachineOptionsSetFeatures` | ❌ | TODO |
| `LLVMTargetMachineOptionsSetABI` | ❌ | TODO |
| `LLVMTargetMachineOptionsSetRelocationModel` | ❌ | TODO |
| `LLVMTargetMachineOptionsSetCodeGenOptLevel` | ❌ | TODO |
| `LLVMCreateTargetMachineWithOptions` | ❌ | TODO |
| `LLVMCreateTargetMachine` | ❌ | TODO |
| `LLVMDisposeTargetMachine` | ❌ | TODO |
| `LLVMGetTargetMachineTarget` | ❌ | TODO |
| `LLVMGetTargetMachineTriple` | ❌ | TODO |
| `LLVMGetTargetMachineCPU` | ❌ | TODO |
| `LLVMGetTargetMachineFeatureString` | ❌ | TODO |
| `LLVMCreateTargetDataLayout` | ❌ | TODO |
| `LLVMSetTargetMachineAsmVerbosity` | ❌ | TODO |
| `LLVMSetTargetMachineFastISel` | ❌ | TODO |
| `LLVMSetTargetMachineGlobalISel` | ❌ | TODO |
| `LLVMSetTargetMachineMachineOutliner` | ❌ | TODO |
| `LLVMTargetMachineEmitToFile` | ❌ | TODO |
| `LLVMTargetMachineEmitToMemoryBuffer` | ❌ | TODO |
| `LLVMAddAnalysisPasses` | ❌ | TODO |

**Proposed API:**

```python
# Proposed (not yet implemented)
target = llvm.get_target_from_triple("x86_64-unknown-linux-gnu")
tm = target.create_target_machine(
    cpu="generic",
    features="",
    opt_level=llvm.CodeGenOptLevel.Default,
    reloc_model=llvm.RelocModel.PIC,
    code_model=llvm.CodeModel.Default
)

# Emit object code
obj_buf = tm.emit_to_memory_buffer(mod, llvm.CodeGenFileType.Object)

# Emit assembly
asm_buf = tm.emit_to_memory_buffer(mod, llvm.CodeGenFileType.Assembly)

# Or to file
tm.emit_to_file(mod, "output.o", llvm.CodeGenFileType.Object)
```

---

## Host Queries

| C API | Python API | Status |
|-------|------------|--------|
| `LLVMGetDefaultTargetTriple` | ❌ | TODO - `llvm.get_default_target_triple()` |
| `LLVMNormalizeTargetTriple` | ❌ | TODO - `llvm.normalize_target_triple(triple)` |
| `LLVMGetHostCPUName` | ❌ | TODO - `llvm.get_host_cpu_name()` |
| `LLVMGetHostCPUFeatures` | ❌ | TODO - `llvm.get_host_cpu_features()` |

**Proposed API:**

```python
# Proposed (not yet implemented)
triple = llvm.get_default_target_triple()  # e.g., "x86_64-unknown-linux-gnu"
cpu = llvm.get_host_cpu_name()  # e.g., "skylake"
features = llvm.get_host_cpu_features()  # e.g., "+avx2,+sse4.2"
```

---

## Target Data (Not Implemented)

| C API | Python API | Status |
|-------|------------|--------|
| `LLVMGetModuleDataLayout` | ❌ | TODO |
| `LLVMSetModuleDataLayout` | ❌ | TODO |
| `LLVMCreateTargetData` | ❌ | TODO |
| `LLVMDisposeTargetData` | ❌ | TODO |
| `LLVMAddTargetLibraryInfo` | ❌ | TODO |
| `LLVMCopyStringRepOfTargetData` | ❌ | TODO |
| `LLVMByteOrder` | ❌ | TODO |
| `LLVMPointerSize` | ❌ | TODO |
| `LLVMPointerSizeForAS` | ❌ | TODO |
| `LLVMIntPtrType` | ❌ | TODO |
| `LLVMIntPtrTypeForAS` | ❌ | TODO |
| `LLVMIntPtrTypeInContext` | ❌ | TODO |
| `LLVMIntPtrTypeForASInContext` | ❌ | TODO |
| `LLVMSizeOfTypeInBits` | ❌ | TODO |
| `LLVMStoreSizeOfType` | ❌ | TODO |
| `LLVMABISizeOfType` | ❌ | TODO |
| `LLVMABIAlignmentOfType` | ❌ | TODO |
| `LLVMCallFrameAlignmentOfType` | ❌ | TODO |
| `LLVMPreferredAlignmentOfType` | ❌ | TODO |
| `LLVMPreferredAlignmentOfGlobal` | ❌ | TODO |
| `LLVMElementAtOffset` | ❌ | TODO |
| `LLVMOffsetOfElement` | ❌ | TODO |

**Proposed API:**

```python
# Proposed (not yet implemented)
target_data = llvm.create_target_data("e-m:e-i64:64-f80:128-n8:16:32:64-S128")

# Query type sizes
size_bits = target_data.size_of_type_in_bits(i32_ty)  # 32
abi_size = target_data.abi_size_of_type(struct_ty)     # bytes
alignment = target_data.abi_alignment_of_type(i64_ty)  # 8

# Get pointer size for address space
ptr_size = target_data.pointer_size(address_space=0)   # 8 for 64-bit

# Get intptr type
intptr = target_data.intptr_type(ctx)  # i64 on 64-bit
```

---

## Summary

| Category | Total | Implemented | Coverage |
|----------|-------|-------------|----------|
| Target Initialization | 10 | 10 | 100% |
| Target Queries | 9 | 7 | 78% |
| Target Machine | 22 | 0 | 0% |
| Host Queries | 4 | 0 | 0% |
| Target Data | 22 | 0 | 0% |
| **Total** | **67** | **17** | **25%** |

---

## Priority Implementation Notes

### High Priority

1. **`LLVMGetDefaultTargetTriple`** - Essential for portability
2. **`LLVMGetHostCPUName`** / **`LLVMGetHostCPUFeatures`** - For optimization
3. **`LLVMCreateTargetMachine`** - Required for code generation
4. **`LLVMTargetMachineEmitToFile`** / **`LLVMTargetMachineEmitToMemoryBuffer`** - Generate object code

### Medium Priority

1. **Target Data queries** - Useful for ABI-correct code generation
2. **`LLVMGetTargetFromTriple`** - Cross-compilation support

### Lower Priority

1. Fine-grained target machine options
2. Analysis passes integration
