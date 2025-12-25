# Miscellaneous Headers Feature Matrix

Implementation status for other LLVM-C headers with Python API mappings.

**Last Updated:** 2024-12-25

## Legend

| Status | Meaning |
|--------|---------|
| ✅ | Implemented |
| ❌ | Not implemented |
| 🚫 | Intentionally skipped |

---

## Analysis.h - Module/Function Verification

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMVerifyModule` | ✅ | `mod.verify()` |
| `LLVMVerifyFunction` | ✅ | `fn.verify()`, `fn.verify_and_print()` |
| `LLVMViewFunctionCFG` | 🚫 | Debugging only, requires graphviz |
| `LLVMViewFunctionCFGOnly` | 🚫 | Debugging only, requires graphviz |

```python
# Verify module
mod.verify()  # Raises exception on error

# Verify function
if not fn.verify():
    success, msg = fn.verify_and_print()
    print(f"Function invalid: {msg}")
```

**Coverage:** 2/4 (50%)

---

## BitReader.h - Bitcode Reading

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMParseBitcode` | 🚫 | Global context |
| `LLVMParseBitcode2` | 🚫 | Global context |
| `LLVMParseBitcodeInContext` | 🚫 | Deprecated |
| `LLVMParseBitcodeInContext2` | ✅ | `ctx.parse_bitcode_from_bytes(data)` |
| `LLVMGetBitcodeModuleInContext` | 🚫 | Deprecated |
| `LLVMGetBitcodeModuleInContext2` | ✅ | `ctx.parse_bitcode_from_bytes(data, lazy=True)` |
| `LLVMGetBitcodeModule` | 🚫 | Global context |
| `LLVMGetBitcodeModule2` | ✅ | Via `ctx.parse_bitcode_from_bytes` |

```python
# Parse bitcode from file
with ctx.parse_bitcode_from_file("module.bc") as mod:
    print(mod)

# Parse bitcode from bytes
with open("module.bc", "rb") as f:
    data = f.read()
with ctx.parse_bitcode_from_bytes(data) as mod:
    print(mod)
```

**Coverage:** 3/8 (37.5%)

---

## BitWriter.h - Bitcode Writing

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMWriteBitcodeToFile` | ✅ | `mod.write_bitcode_to_file(path)` |
| `LLVMWriteBitcodeToMemoryBuffer` | ✅ | `mod.write_bitcode_to_memory_buffer()` → `bytes` |
| `LLVMWriteBitcodeToFD` | 🚫 | Low-level file descriptor API |
| `LLVMWriteBitcodeToFileHandle` | 🚫 | Deprecated |

```python
# Write bitcode to file
mod.write_bitcode_to_file("output.bc")

# Write bitcode to bytes
bc_data = mod.write_bitcode_to_memory_buffer()
with open("output.bc", "wb") as f:
    f.write(bc_data)
```

**Coverage:** 2/4 (50%, 100% of useful APIs)

---

## IRReader.h - IR Parsing

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMParseIRInContext` | ✅ | `ctx.parse_ir(source, mod_name)` |

```python
ir_source = """
define i32 @add(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b
  ret i32 %sum
}
"""

with ctx.parse_ir(ir_source, "my_module") as mod:
    print(mod)
```

**Coverage:** 1/1 (100%)

---

## PassBuilder.h - New Pass Manager

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMCreatePassBuilderOptions` | ✅ | `llvm.PassBuilderOptions()` |
| `LLVMDisposePassBuilderOptions` | ✅ | Automatic cleanup |
| `LLVMPassBuilderOptionsSetVerifyEach` | ✅ | `opts.set_verify_each(bool)` |
| `LLVMPassBuilderOptionsSetDebugLogging` | ✅ | `opts.set_debug_logging(bool)` |
| `LLVMPassBuilderOptionsSetLoopInterleaving` | ✅ | `opts.set_loop_interleaving(bool)` |
| `LLVMPassBuilderOptionsSetLoopVectorization` | ✅ | `opts.set_loop_vectorization(bool)` |
| `LLVMPassBuilderOptionsSetSLPVectorization` | ✅ | `opts.set_slp_vectorization(bool)` |
| `LLVMPassBuilderOptionsSetLoopUnrolling` | ✅ | `opts.set_loop_unrolling(bool)` |
| `LLVMPassBuilderOptionsSetForgetAllSCEVInLoopUnroll` | ✅ | `opts.set_forget_all_scev_in_loop_unroll(bool)` |
| `LLVMPassBuilderOptionsSetLicmMssaOptCap` | ✅ | `opts.set_licm_mssa_opt_cap(int)` |
| `LLVMPassBuilderOptionsSetLicmMssaNoAccForPromotionCap` | ✅ | `opts.set_licm_mssa_no_acc_for_promotion_cap(int)` |
| `LLVMPassBuilderOptionsSetCallGraphProfile` | ✅ | `opts.set_call_graph_profile(bool)` |
| `LLVMPassBuilderOptionsSetMergeFunctions` | ✅ | `opts.set_merge_functions(bool)` |
| `LLVMPassBuilderOptionsSetInlinerThreshold` | ✅ | `opts.set_inliner_threshold(int)` |
| `LLVMRunPasses` | ✅ | `llvm.run_passes(mod, passes, tm, opts)` |

```python
# Create pass builder options
opts = llvm.PassBuilderOptions()
opts.set_loop_vectorization(True)
opts.set_slp_vectorization(True)
opts.set_inliner_threshold(250)

# Run optimization passes
llvm.run_passes(mod, "default<O2>", target_machine, opts)

# Standard optimization levels
llvm.run_passes(mod, "default<O0>", tm, opts)  # No optimization
llvm.run_passes(mod, "default<O1>", tm, opts)  # Light optimization
llvm.run_passes(mod, "default<O2>", tm, opts)  # Standard optimization
llvm.run_passes(mod, "default<O3>", tm, opts)  # Aggressive optimization
llvm.run_passes(mod, "default<Os>", tm, opts)  # Size optimization
llvm.run_passes(mod, "default<Oz>", tm, opts)  # Aggressive size

# Custom pass pipeline
llvm.run_passes(mod, "function(simplifycfg,instcombine)", tm, opts)
```

**Coverage:** 15/15 (100%)

---

## Linker.h - Module Linking

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMLinkModules2` | ✅ | `mod.link_module(other_mod)` |

```python
# Link modules (other_mod is consumed/destroyed)
mod.link_module(other_mod)
```

**Coverage:** 1/1 (100%)

---

## Disassembler.h - Instruction Disassembly

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMCreateDisasm` | ❌ | Use `LLVMCreateDisasmCPUFeatures` |
| `LLVMCreateDisasmCPU` | ❌ | Use `LLVMCreateDisasmCPUFeatures` |
| `LLVMCreateDisasmCPUFeatures` | ✅ | `llvm.create_disassembler(triple, cpu, features)` |
| `LLVMSetDisasmOptions` | ✅ | `disasm.set_options(options)` |
| `LLVMDisasmDispose` | ✅ | Automatic cleanup |
| `LLVMDisasmInstruction` | ✅ | `disasm.disassemble(bytes, pc)` |

```python
# Initialize disassembler support
llvm.initialize_all_disassemblers()

# Create disassembler for x86-64
disasm = llvm.create_disassembler("x86_64-unknown-linux-gnu", "", "")

# Disassemble bytes
code = bytes([0x48, 0x89, 0xe5])  # mov rbp, rsp
text, size = disasm.disassemble(code, 0x1000)
print(f"{text} ({size} bytes)")
```

**Coverage:** 3/6 (50%)

---

## Object.h - Object File Handling

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMCreateBinary` | ✅ | `llvm.create_binary(buf, ctx)` |
| `LLVMDisposeBinary` | ✅ | Automatic cleanup |
| `LLVMBinaryCopyMemoryBuffer` | ❌ | TODO |
| `LLVMBinaryGetType` | ✅ | `binary.type` |
| `LLVMObjectFileCopySectionIterator` | ✅ | `binary.sections()` |
| `LLVMObjectFileIsSectionIteratorAtEnd` | ✅ | Via iterator |
| `LLVMMoveToNextSection` | ✅ | Via iterator |
| `LLVMObjectFileCopySymbolIterator` | ✅ | `binary.symbols()` |
| `LLVMObjectFileIsSymbolIteratorAtEnd` | ✅ | Via iterator |
| `LLVMMoveToNextSymbol` | ✅ | Via iterator |
| `LLVMGetSectionName` | ✅ | `section.name` |
| `LLVMGetSectionSize` | ✅ | `section.size` |
| `LLVMGetSectionContents` | ✅ | `section.contents` |
| `LLVMGetSectionAddress` | ✅ | `section.address` |
| `LLVMGetSectionContainsSymbol` | ❌ | TODO |
| `LLVMGetSymbolName` | ✅ | `symbol.name` |
| `LLVMGetSymbolAddress` | ✅ | `symbol.address` |
| `LLVMGetSymbolSize` | ✅ | `symbol.size` |
| `LLVMGetRelocations` | ✅ | `section.relocations()` |
| `LLVMGetRelocationOffset` | ✅ | `reloc.offset` |
| `LLVMGetRelocationSymbol` | ✅ | `reloc.symbol` |
| `LLVMGetRelocationType` | ✅ | `reloc.type` |
| `LLVMGetRelocationTypeName` | ✅ | `reloc.type_name` |
| `LLVMGetRelocationValueString` | ✅ | `reloc.value_string` |

```python
# Read object file
buf = llvm.MemoryBuffer.from_file("module.o")
binary = llvm.create_binary(buf)

for section in binary.sections():
    print(f"Section: {section.name}, Size: {section.size}")

for symbol in binary.symbols():
    print(f"Symbol: {symbol.name} @ 0x{symbol.address:x}")
```

**Coverage:** 23/31 (74%)

---

## Error.h - Error Handling

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMGetErrorTypeId` | 🚫 | Errors → Python exceptions |
| `LLVMConsumeError` | 🚫 | Automatic in bindings |
| `LLVMCantFail` | 🚫 | Not needed in Python |
| `LLVMGetErrorMessage` | 🚫 | In exception messages |
| `LLVMDisposeErrorMessage` | 🚫 | Automatic |
| `LLVMGetStringErrorTypeId` | 🚫 | Not needed |
| `LLVMCreateStringError` | 🚫 | Not needed |

Note: LLVM errors are converted to Python exceptions (`LLVMError`, `LLVMParseError`, `LLVMMemoryError`)

**Coverage:** N/A (by design)

---

## ErrorHandling.h - Fatal Error Handling

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMInstallFatalErrorHandler` | ❌ | TODO |
| `LLVMResetFatalErrorHandler` | ❌ | TODO |
| `LLVMEnablePrettyStackTrace` | ❌ | TODO |

**Coverage:** 0/3 (0%)

---

## Support.h - Miscellaneous Support

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMLoadLibraryPermanently` | ❌ | TODO |
| `LLVMParseCommandLineOptions` | ❌ | TODO |
| `LLVMSearchForAddressOfSymbol` | ❌ | TODO |
| `LLVMAddSymbol` | ❌ | TODO |

**Coverage:** 0/4 (0%)

---

## Comdat.h - COMDAT Handling

| C API | Status | Python API |
|-------|--------|------------|
| `LLVMGetOrInsertComdat` | ❌ | TODO |
| `LLVMGetComdat` | ❌ | TODO |
| `LLVMSetComdat` | ❌ | TODO |
| `LLVMGetComdatSelectionKind` | ❌ | TODO |
| `LLVMSetComdatSelectionKind` | ❌ | TODO |

**Coverage:** 0/5 (0%)

---

## Summary

| Header | Total | ✅ Impl | 🚫 Skip | ❌ TODO | Coverage |
|--------|-------|---------|---------|---------|----------|
| Analysis.h | 4 | 2 | 2 | 0 | 50% |
| BitReader.h | 8 | 3 | 5 | 0 | 37.5% |
| BitWriter.h | 4 | 2 | 2 | 0 | 50% |
| IRReader.h | 1 | 1 | 0 | 0 | 100% |
| PassBuilder.h | 15 | 15 | 0 | 0 | 100% |
| Linker.h | 1 | 1 | 0 | 0 | 100% |
| Disassembler.h | 6 | 3 | 0 | 3 | 50% |
| Object.h | 31 | 23 | 0 | 8 | 74% |
| Error.h | 7 | 0 | 7 | 0 | N/A |
| ErrorHandling.h | 3 | 0 | 0 | 3 | 0% |
| Support.h | 4 | 0 | 0 | 4 | 0% |
| Comdat.h | 5 | 0 | 0 | 5 | 0% |
| **Total** | **89** | **50** | **16** | **23** | **73%** |

---

## Remaining TODO - Detailed

### Disassembler.h (2 remaining)
| Function | Description | Priority |
|----------|-------------|----------|
| `LLVMCreateDisasm` | Create basic disassembler | Low - use CPU/features version |
| `LLVMCreateDisasmCPU` | Create with CPU | Low - use CPU/features version |

### Object.h (8 remaining)
| Function | Description | Priority |
|----------|-------------|----------|
| `LLVMBinaryCopyMemoryBuffer` | Copy binary to memory buffer | Low |
| `LLVMGetSectionContainsSymbol` | Check if section contains symbol | Medium |
| `LLVMDisposeSectionIterator` | Dispose section iterator | Automatic |
| `LLVMDisposeSymbolIterator` | Dispose symbol iterator | Automatic |
| `LLVMDisposeRelocationIterator` | Dispose reloc iterator | Automatic |
| `LLVMMoveToContainingSection` | Move to containing section | Low |
| `LLVMGetSectionIndex` | Get section index | Low |
| `LLVMGetRelocationAddress` | Get relocation address | Low |

### ErrorHandling.h (3 remaining)
| Function | Description | Priority | Notes |
|----------|-------------|----------|-------|
| `LLVMInstallFatalErrorHandler` | Install custom fatal error handler | Low | Advanced: custom crash handling |
| `LLVMResetFatalErrorHandler` | Reset to default handler | Low | Companion to above |
| `LLVMEnablePrettyStackTrace` | Enable pretty stack traces | Low | Debugging only |

**Use Case:** Handle LLVM fatal errors gracefully instead of crashing. Useful for long-running services.

### Support.h (4 remaining)
| Function | Description | Priority | Notes |
|----------|-------------|----------|-------|
| `LLVMLoadLibraryPermanently` | Load shared library into process | Low | JIT: load native libs |
| `LLVMParseCommandLineOptions` | Parse LLVM command line options | Low | Set LLVM internal flags |
| `LLVMSearchForAddressOfSymbol` | Find symbol in loaded libraries | Low | JIT: symbol resolution |
| `LLVMAddSymbol` | Add symbol to global symbol table | Low | JIT: symbol injection |

**Use Case:** JIT symbol resolution. When JIT-compiled code needs to call external functions.

### Comdat.h (5 remaining)
| Function | Description | Priority | Notes |
|----------|-------------|----------|-------|
| `LLVMGetOrInsertComdat` | Get or create COMDAT section | Low | Windows linking |
| `LLVMGetComdat` | Get COMDAT for global | Low | Windows linking |
| `LLVMSetComdat` | Set COMDAT for global | Low | Windows linking |
| `LLVMGetComdatSelectionKind` | Get COMDAT selection kind | Low | Windows linking |
| `LLVMSetComdatSelectionKind` | Set COMDAT selection kind | Low | Windows linking |

**Use Case:** COMDAT sections control symbol deduplication in Windows COFF object files. Required for proper linking of C++ template instantiations and inline functions on Windows.

**Proposed API:**
```python
# Create COMDAT
comdat = mod.get_or_insert_comdat("my_section")
comdat.selection_kind = llvm.ComdatSelectionKind.Any  # or Largest, NoDuplicates, SameSize, ExactMatch

# Associate with globals
fn.comdat = comdat
gv.comdat = comdat
```

---

## Advanced Features Not Yet Tracked

### ORC JIT (Orc.h, LLJIT.h, OrcEE.h, LLJITUtils.h)

Modern JIT compilation infrastructure (~90 functions total):

| Category | Functions | Description |
|----------|-----------|-------------|
| ExecutionSession | ~15 | JIT session management |
| JITDylib | ~10 | Dynamic library abstraction |
| MaterializationUnit | ~8 | Lazy compilation units |
| ObjectLayer | ~12 | Object file linking |
| IRLayer | ~8 | IR compilation |
| LLJIT | ~20 | High-level JIT interface |
| LookupState | ~5 | Symbol lookup |
| ThreadSafeContext | ~5 | Thread-safe context |

**Use Case:** JIT compile and execute LLVM IR at runtime. Useful for:
- Language interpreters with JIT
- Runtime code generation
- Dynamic optimization

**Priority:** Medium - implement if JIT functionality is needed.

### ExecutionEngine.h (Legacy JIT)

Legacy JIT interface (~38 functions). **Prefer ORC JIT instead.**

| Function Group | Count | Notes |
|----------------|-------|-------|
| Create/Dispose | 8 | Create execution engines |
| Run Functions | 6 | Execute JIT code |
| Global Access | 8 | Access global variables |
| Target Data | 4 | Get target info |
| Function Access | 6 | Find/add functions |
| Misc | 6 | Other utilities |

**Status:** Not planned. Use ORC JIT for new code.

### Remarks.h (Optimization Remarks)

Optimization diagnostics (~24 functions):

| Category | Functions | Description |
|----------|-----------|-------------|
| Parser | 8 | Parse remark files |
| Entry | 10 | Access remark entries |
| String | 4 | Remark strings |
| Debug Location | 2 | Source locations |

**Use Case:** Analyze optimization decisions for performance tuning.

**Priority:** Low - specialized debugging use case.
