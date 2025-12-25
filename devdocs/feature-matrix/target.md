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
llvm.initialize_native_target()
llvm.initialize_native_asm_printer()

# Get target for host
triple = llvm.get_default_target_triple()
target = llvm.get_target_from_triple(triple)

# Create target machine
tm = llvm.create_target_machine(
    target, triple,
    cpu=llvm.get_host_cpu_name(),
    features=llvm.get_host_cpu_features(),
    opt_level=llvm.CodeGenOptLevel.Default,
    reloc_mode=llvm.RelocMode.Default,
    code_model=llvm.CodeModel.Default
)

# Get data layout
td = tm.create_data_layout()
print(f"Pointer size: {td.pointer_size()} bytes")

# Emit object code
obj_bytes = tm.emit_to_memory_buffer(mod, llvm.CodeGenFileType.Object)
```

---

## Target Initialization

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMInitializeAllTargetInfos` | ✅ | `llvm.initialize_all_target_infos()` |
| `LLVMInitializeAllTargets` | ✅ | `llvm.initialize_all_targets()` |
| `LLVMInitializeAllTargetMCs` | ✅ | `llvm.initialize_all_target_mcs()` |
| `LLVMInitializeAllAsmPrinters` | ✅ | `llvm.initialize_all_asm_printers()` |
| `LLVMInitializeAllAsmParsers` | ✅ | `llvm.initialize_all_asm_parsers()` |
| `LLVMInitializeAllDisassemblers` | ✅ | `llvm.initialize_all_disassemblers()` |
| `LLVMInitializeNativeTarget` | ✅ | `llvm.initialize_native_target()` |
| `LLVMInitializeNativeAsmPrinter` | ✅ | `llvm.initialize_native_asm_printer()` |
| `LLVMInitializeNativeAsmParser` | ✅ | `llvm.initialize_native_asm_parser()` |
| `LLVMInitializeNativeDisassembler` | ✅ | `llvm.initialize_native_disassembler()` |

---

## Target Queries

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMGetFirstTarget` | ✅ | `llvm.get_first_target()` |
| `LLVMGetNextTarget` | ✅ | `target.next_target` |
| `LLVMGetTargetFromName` | ✅ | `llvm.get_target_from_name(name)` |
| `LLVMGetTargetFromTriple` | ✅ | `llvm.get_target_from_triple(triple)` |
| `LLVMGetTargetName` | ✅ | `target.name` |
| `LLVMGetTargetDescription` | ✅ | `target.description` |
| `LLVMTargetHasJIT` | ✅ | `target.has_jit` |
| `LLVMTargetHasTargetMachine` | ✅ | `target.has_target_machine` |
| `LLVMTargetHasAsmBackend` | ✅ | `target.has_asm_backend` |

```python
# Iterate all targets
for target in llvm.targets():
    print(f"{target.name}: {target.description}")
```

---

## Host Queries

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMGetDefaultTargetTriple` | ✅ | `llvm.get_default_target_triple()` |
| `LLVMNormalizeTargetTriple` | ✅ | `llvm.normalize_target_triple(triple)` |
| `LLVMGetHostCPUName` | ✅ | `llvm.get_host_cpu_name()` |
| `LLVMGetHostCPUFeatures` | ✅ | `llvm.get_host_cpu_features()` |

```python
triple = llvm.get_default_target_triple()  # e.g., "arm64-apple-macosx15.0.0"
cpu = llvm.get_host_cpu_name()             # e.g., "apple-m1"
features = llvm.get_host_cpu_features()    # e.g., "+neon,+fp-armv8,..."
```

---

## Target Machine

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMCreateTargetMachine` | ✅ | `llvm.create_target_machine(target, triple, cpu, features, opt, reloc, model)` |
| `LLVMDisposeTargetMachine` | ✅ | Automatic cleanup |
| `LLVMGetTargetMachineTarget` | ✅ | `tm.target` |
| `LLVMGetTargetMachineTriple` | ✅ | `tm.triple` |
| `LLVMGetTargetMachineCPU` | ✅ | `tm.cpu` |
| `LLVMGetTargetMachineFeatureString` | ✅ | `tm.feature_string` |
| `LLVMCreateTargetDataLayout` | ✅ | `tm.create_data_layout()` |
| `LLVMSetTargetMachineAsmVerbosity` | ✅ | `tm.set_asm_verbosity(bool)` |
| `LLVMSetTargetMachineFastISel` | ✅ | `tm.set_fast_isel(bool)` |
| `LLVMSetTargetMachineGlobalISel` | ✅ | `tm.set_global_isel(bool)` |
| `LLVMSetTargetMachineGlobalISelAbort` | ✅ | `tm.set_global_isel_abort(mode)` |
| `LLVMSetTargetMachineMachineOutliner` | ✅ | `tm.set_machine_outliner(bool)` |
| `LLVMTargetMachineEmitToFile` | ✅ | `tm.emit_to_file(mod, path, file_type)` |
| `LLVMTargetMachineEmitToMemoryBuffer` | ✅ | `tm.emit_to_memory_buffer(mod, file_type)` → `bytes` |
| `LLVMCreateTargetMachineOptions` | ❌ | Not implemented (use create_target_machine) |
| `LLVMDisposeTargetMachineOptions` | ❌ | Not implemented |
| `LLVMTargetMachineOptionsSetCPU` | ❌ | Not implemented |
| `LLVMTargetMachineOptionsSetFeatures` | ❌ | Not implemented |
| `LLVMTargetMachineOptionsSetABI` | ❌ | Not implemented |
| `LLVMTargetMachineOptionsSetRelocationModel` | ❌ | Not implemented |
| `LLVMTargetMachineOptionsSetCodeGenOptLevel` | ❌ | Not implemented |
| `LLVMCreateTargetMachineWithOptions` | ❌ | Not implemented |
| `LLVMAddAnalysisPasses` | ❌ | Not implemented (legacy PM) |

```python
# Create target machine
llvm.initialize_native_target()
llvm.initialize_native_asm_printer()

target = llvm.get_target_from_triple(llvm.get_default_target_triple())
tm = llvm.create_target_machine(
    target, 
    llvm.get_default_target_triple(),
    llvm.get_host_cpu_name(),
    llvm.get_host_cpu_features(),
    llvm.CodeGenOptLevel.Default,
    llvm.RelocMode.Default,
    llvm.CodeModel.Default
)

# Emit to file
tm.emit_to_file(mod, "output.o", llvm.CodeGenFileType.Object)
tm.emit_to_file(mod, "output.s", llvm.CodeGenFileType.Assembly)

# Emit to memory
obj_bytes = tm.emit_to_memory_buffer(mod, llvm.CodeGenFileType.Object)
asm_bytes = tm.emit_to_memory_buffer(mod, llvm.CodeGenFileType.Assembly)
```

---

## Target Data

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMCreateTargetData` | ✅ | `llvm.create_target_data(string_rep)` |
| `LLVMDisposeTargetData` | ✅ | Automatic cleanup |
| `LLVMCopyStringRepOfTargetData` | ✅ | `str(td)` |
| `LLVMByteOrder` | ✅ | `td.byte_order` |
| `LLVMPointerSize` | ✅ | `td.pointer_size()` |
| `LLVMPointerSizeForAS` | ✅ | `td.pointer_size(address_space)` |
| `LLVMIntPtrType` | ✅ | `td.int_ptr_type(ctx)` |
| `LLVMIntPtrTypeForAS` | ✅ | `td.int_ptr_type(ctx, address_space)` |
| `LLVMIntPtrTypeInContext` | ✅ | `td.int_ptr_type(ctx)` |
| `LLVMIntPtrTypeForASInContext` | ✅ | `td.int_ptr_type(ctx, address_space)` |
| `LLVMSizeOfTypeInBits` | ✅ | `td.size_of_type_in_bits(ty)` |
| `LLVMStoreSizeOfType` | ✅ | `td.store_size_of_type(ty)` |
| `LLVMABISizeOfType` | ✅ | `td.abi_size_of_type(ty)` |
| `LLVMABIAlignmentOfType` | ✅ | `td.abi_alignment_of_type(ty)` |
| `LLVMCallFrameAlignmentOfType` | ✅ | `td.call_frame_alignment_of_type(ty)` |
| `LLVMPreferredAlignmentOfType` | ✅ | `td.preferred_alignment_of_type(ty)` |
| `LLVMPreferredAlignmentOfGlobal` | ✅ | `td.preferred_alignment_of_global(gv)` |
| `LLVMElementAtOffset` | ✅ | `td.element_at_offset(struct_ty, offset)` |
| `LLVMOffsetOfElement` | ✅ | `td.offset_of_element(struct_ty, elem)` |
| `LLVMGetModuleDataLayout` | ❌ | Not implemented |
| `LLVMSetModuleDataLayout` | ❌ | Not implemented |
| `LLVMAddTargetLibraryInfo` | ❌ | Not implemented (legacy PM) |

```python
# Get data layout from target machine
td = tm.create_data_layout()
print(f"Data layout: {td}")

# Or create directly
td = llvm.create_target_data("e-m:o-i64:64-f80:128-n8:16:32:64-S128")

# Query sizes
print(f"Pointer size: {td.pointer_size()} bytes")
print(f"i64 size: {td.size_of_type_in_bits(ctx.int64_type())} bits")
print(f"i64 alignment: {td.abi_alignment_of_type(ctx.int64_type())} bytes")

# Get integer type matching pointer size
int_ptr = td.int_ptr_type(ctx)  # Returns i64 on 64-bit
```

---

## Enums

| C Enum | Python Enum |
|--------|-------------|
| `LLVMCodeGenOptLevel` | `llvm.CodeGenOptLevel` (.None_, .Less, .Default, .Aggressive) |
| `LLVMRelocMode` | `llvm.RelocMode` (.Default, .Static, .PIC, .DynamicNoPic, .ROPI, .RWPI, .ROPI_RWPI) |
| `LLVMCodeModel` | `llvm.CodeModel` (.Default, .JITDefault, .Tiny, .Small, .Kernel, .Medium, .Large) |
| `LLVMCodeGenFileType` | `llvm.CodeGenFileType` (.Assembly, .Object) |
| `LLVMGlobalISelAbortMode` | `llvm.GlobalISelAbortMode` (.Disable, .Enable, .DisableWithDiag) |
| `LLVMByteOrdering` | `llvm.ByteOrdering` (.BigEndian, .LittleEndian) |

---

## Summary

| Category | Total | Implemented | Coverage |
|----------|-------|-------------|----------|
| Target Initialization | 10 | 10 | 100% |
| Target Queries | 9 | 9 | 100% |
| Host Queries | 4 | 4 | 100% |
| Target Machine | 23 | 14 | 61% |
| Target Data | 22 | 19 | 86% |
| **Total** | **68** | **56** | **82%** |

---

## Not Implemented (Low Priority)

1. **TargetMachineOptions API** - Alternative builder pattern for target machine creation. Current `create_target_machine()` is sufficient.
2. **Module data layout get/set** - Can be added if needed.
3. **Legacy PassManager integration** - Use new PassBuilder API instead.
