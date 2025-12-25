# Miscellaneous Headers Feature Matrix

Implementation status for other LLVM-C headers with Python API mappings.

## Legend

| Status | Meaning |
|--------|---------|
| ✅ | Implemented |
| ❌ | Not implemented |

---

## Analysis.h - Module Verification

| C API | Python API | Example |
|-------|------------|---------|
| `LLVMVerifyModule` | `mod.verify()` | `if not mod.verify(): print(mod.get_verification_error())` |
| `LLVMVerifyFunction` | ❌ | TODO - `fn.verify()` |
| `LLVMViewFunctionCFG` | ❌ | Debugging only |
| `LLVMViewFunctionCFGOnly` | ❌ | Debugging only |

```python
# Verify module
if not mod.verify():
    error = mod.get_verification_error()
    print(f"Module verification failed: {error}")
```

**Coverage:** 1/4 (25%)

---

## BitReader.h - Bitcode Reading

| C API | Python API | Example |
|-------|------------|---------|
| `LLVMParseBitcode` | 🚫 | Global context |
| `LLVMParseBitcode2` | 🚫 | Global context |
| `LLVMParseBitcodeInContext` | 🚫 | Deprecated |
| `LLVMParseBitcodeInContext2` | `ctx.parse_bitcode_from_bytes(data)` | See below |
| `LLVMGetBitcodeModuleInContext` | 🚫 | Deprecated |
| `LLVMGetBitcodeModuleInContext2` | `ctx.parse_bitcode_from_bytes(data, lazy=True)` | See below |
| `LLVMGetBitcodeModule` | 🚫 | Global context |
| `LLVMGetBitcodeModule2` | Via `ctx.parse_bitcode_from_bytes` | - |

```python
# Parse bitcode from file
with ctx.parse_bitcode_from_file("module.bc") as mod:
    print(mod)

# Parse bitcode from bytes
with open("module.bc", "rb") as f:
    data = f.read()
with ctx.parse_bitcode_from_bytes(data) as mod:
    print(mod)

# Lazy loading (for large modules)
with ctx.parse_bitcode_from_file("large.bc", lazy=True) as mod:
    # Only referenced functions are loaded
    fn = mod.get_function("main")
```

**Coverage:** 3/8 (37.5%)

---

## BitWriter.h - Bitcode Writing (Not Implemented)

| C API | Python API | Status |
|-------|------------|--------|
| `LLVMWriteBitcodeToFile` | ❌ | TODO - `mod.write_bitcode_to_file(path)` |
| `LLVMWriteBitcodeToFD` | ❌ | TODO |
| `LLVMWriteBitcodeToFileHandle` | ❌ | TODO |
| `LLVMWriteBitcodeToMemoryBuffer` | ❌ | TODO - `mod.write_bitcode_to_bytes()` |

**Proposed API:**

```python
# Proposed (not yet implemented)

# Write bitcode to file
mod.write_bitcode_to_file("output.bc")

# Write bitcode to bytes
bc_data = mod.write_bitcode_to_bytes()
with open("output.bc", "wb") as f:
    f.write(bc_data)
```

**Coverage:** 0/4 (0%)

---

## IRReader.h - IR Parsing

| C API | Python API | Example |
|-------|------------|---------|
| `LLVMParseIRInContext` | `ctx.parse_ir(source, mod_name)` | See below |

```python
# Parse LLVM IR from string
ir_source = """
define i32 @add(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b
  ret i32 %sum
}
"""

try:
    with ctx.parse_ir(ir_source, "my_module") as mod:
        print(mod)
except llvm.LLVMParseError as e:
    print(f"Parse error: {e}")
    for diag in ctx.get_diagnostics():
        print(f"  {diag.severity}: {diag.message}")
```

**Coverage:** 1/1 (100%)

---

## Disassembler.h - Instruction Disassembly

| C API | Python API | Example |
|-------|------------|---------|
| `LLVMCreateDisasm` | ❌ | Use `LLVMCreateDisasmCPUFeatures` |
| `LLVMCreateDisasmCPU` | ❌ | Use `LLVMCreateDisasmCPUFeatures` |
| `LLVMCreateDisasmCPUFeatures` | `llvm.create_disassembler(triple, cpu, features)` | See below |
| `LLVMSetDisasmOptions` | ❌ | TODO |
| `LLVMDisasmDispose` | Automatic (destructor) | - |
| `LLVMDisasmInstruction` | `disasm.disassemble(bytes, pc)` | See below |

```python
# Initialize disassembler support
llvm.initialize_all_targets()
llvm.initialize_all_target_mcs()
llvm.initialize_all_disassemblers()

# Create disassembler for x86-64
disasm = llvm.create_disassembler("x86_64-unknown-linux-gnu", "", "")

# Disassemble bytes
code = bytes([0x48, 0x89, 0xe5])  # mov rbp, rsp
text, size = disasm.disassemble(code, 0x1000)
print(f"{text} ({size} bytes)")  # "movq %rsp, %rbp (3 bytes)"
```

**Coverage:** 3/6 (50%)

---

## Object.h - Object File Handling

| C API | Python API | Example |
|-------|------------|---------|
| `LLVMCreateBinary` | `llvm.create_binary(buf, ctx)` | See below |
| `LLVMDisposeBinary` | Automatic (destructor) | - |
| `LLVMBinaryCopyMemoryBuffer` | ❌ | TODO |
| `LLVMBinaryGetType` | `binary.type` | `if binary.type == llvm.BinaryType.Object:` |
| `LLVMObjectFileCopySectionIterator` | `binary.sections()` | See below |
| `LLVMObjectFileIsSectionIteratorAtEnd` | Via iterator protocol | - |
| `LLVMMoveToNextSection` | Via iterator protocol | - |
| `LLVMObjectFileCopySymbolIterator` | `binary.symbols()` | See below |
| `LLVMObjectFileIsSymbolIteratorAtEnd` | Via iterator protocol | - |
| `LLVMMoveToNextSymbol` | Via iterator protocol | - |
| `LLVMGetSectionName` | `section.name` | `name = section.name` |
| `LLVMGetSectionSize` | `section.size` | `size = section.size` |
| `LLVMGetSectionContents` | `section.contents` | `data = section.contents` |
| `LLVMGetSectionAddress` | `section.address` | `addr = section.address` |
| `LLVMGetSectionContainsSymbol` | ❌ | TODO |
| `LLVMGetSymbolName` | `symbol.name` | `name = symbol.name` |
| `LLVMGetSymbolAddress` | `symbol.address` | `addr = symbol.address` |
| `LLVMGetSymbolSize` | `symbol.size` | `size = symbol.size` |
| `LLVMGetRelocations` | `section.relocations()` | - |
| `LLVMGetRelocationOffset` | `reloc.offset` | - |
| `LLVMGetRelocationSymbol` | `reloc.symbol` | - |
| `LLVMGetRelocationType` | `reloc.type` | - |
| `LLVMGetRelocationTypeName` | `reloc.type_name` | - |
| `LLVMGetRelocationValueString` | `reloc.value_string` | - |

```python
# Read object file
buf = llvm.MemoryBuffer.from_file("module.o")
with ctx:
    binary = llvm.create_binary(buf, ctx)
    
    print(f"Binary type: {binary.type}")
    
    # Iterate sections
    for section in binary.sections():
        print(f"Section: {section.name}")
        print(f"  Size: {section.size}")
        print(f"  Address: 0x{section.address:x}")
    
    # Iterate symbols
    for symbol in binary.symbols():
        print(f"Symbol: {symbol.name} @ 0x{symbol.address:x}")
```

**Coverage:** 23/31 (74%)

---

## PassBuilder.h - New Pass Manager (Not Implemented)

| C API | Python API | Status |
|-------|------------|--------|
| `LLVMCreatePassBuilderOptions` | ❌ | TODO |
| `LLVMDisposePassBuilderOptions` | ❌ | TODO |
| `LLVMPassBuilderOptionsSetLoopInterleaving` | ❌ | TODO |
| `LLVMPassBuilderOptionsSetLoopVectorization` | ❌ | TODO |
| `LLVMPassBuilderOptionsSetSLPVectorization` | ❌ | TODO |
| `LLVMPassBuilderOptionsSetLoopUnrolling` | ❌ | TODO |
| `LLVMPassBuilderOptionsSetForgetAllSCEVInLoopUnroll` | ❌ | TODO |
| `LLVMPassBuilderOptionsSetLicmMssaOptCap` | ❌ | TODO |
| `LLVMPassBuilderOptionsSetLicmMssaNoAccForPromotionCap` | ❌ | TODO |
| `LLVMPassBuilderOptionsSetCallGraphProfile` | ❌ | TODO |
| `LLVMPassBuilderOptionsSetMergeFunctions` | ❌ | TODO |
| `LLVMPassBuilderOptionsSetInlinerThreshold` | ❌ | TODO |
| `LLVMPassBuilderOptionsSetVerifyEach` | ❌ | TODO |
| `LLVMPassBuilderOptionsSetDebugLogging` | ❌ | TODO |
| `LLVMRunPasses` | ❌ | TODO |

**Proposed API:**

```python
# Proposed (not yet implemented)

# Create pass builder options
opts = llvm.create_pass_builder_options()
opts.loop_vectorization = True
opts.slp_vectorization = True
opts.inliner_threshold = 250

# Run optimization passes
llvm.run_passes(mod, "default<O2>", target_machine, opts)

# Or with a custom pass pipeline
llvm.run_passes(mod, "function(simplifycfg,instcombine)", target_machine, opts)

# Standard optimization levels
llvm.run_passes(mod, "default<O0>", tm)  # No optimization
llvm.run_passes(mod, "default<O1>", tm)  # Light optimization
llvm.run_passes(mod, "default<O2>", tm)  # Standard optimization
llvm.run_passes(mod, "default<O3>", tm)  # Aggressive optimization
llvm.run_passes(mod, "default<Os>", tm)  # Size optimization
llvm.run_passes(mod, "default<Oz>", tm)  # Aggressive size optimization
```

**Coverage:** 0/15 (0%)

---

## Linker.h - Module Linking (Not Implemented)

| C API | Python API | Status |
|-------|------------|--------|
| `LLVMLinkModules2` | ❌ | TODO - `mod.link(other_mod)` |

**Proposed API:**

```python
# Proposed (not yet implemented)

# Link modules (other_mod is consumed)
mod.link(other_mod)

# Or with explicit control
mod.link(other_mod, preserve_source=False)
```

**Coverage:** 0/1 (0%)

---

## Error.h - Error Handling (Not Implemented)

| C API | Python API | Status |
|-------|------------|--------|
| `LLVMGetErrorTypeId` | ❌ | Errors are converted to exceptions |
| `LLVMConsumeError` | ❌ | Automatic in bindings |
| `LLVMCantFail` | ❌ | Not needed in Python |
| `LLVMGetErrorMessage` | ❌ | Error messages in exceptions |
| `LLVMDisposeErrorMessage` | ❌ | Automatic |
| `LLVMGetStringErrorTypeId` | ❌ | Not needed |
| `LLVMCreateStringError` | ❌ | Not needed |

Note: LLVM errors are automatically converted to Python exceptions (`LLVMError`, `LLVMParseError`, etc.)

**Coverage:** 0/7 (0% - by design, using exceptions instead)

---

## ErrorHandling.h - Fatal Error Handling (Not Implemented)

| C API | Python API | Status |
|-------|------------|--------|
| `LLVMInstallFatalErrorHandler` | ❌ | TODO |
| `LLVMResetFatalErrorHandler` | ❌ | TODO |
| `LLVMEnablePrettyStackTrace` | ❌ | TODO |

**Coverage:** 0/3 (0%)

---

## Support.h - Miscellaneous Support (Not Implemented)

| C API | Python API | Status |
|-------|------------|--------|
| `LLVMLoadLibraryPermanently` | ❌ | TODO |
| `LLVMParseCommandLineOptions` | ❌ | TODO |
| `LLVMSearchForAddressOfSymbol` | ❌ | TODO |
| `LLVMAddSymbol` | ❌ | TODO |

**Coverage:** 0/4 (0%)

---

## Comdat.h - COMDAT Handling (Not Implemented)

| C API | Python API | Status |
|-------|------------|--------|
| `LLVMGetOrInsertComdat` | ❌ | TODO - `mod.get_or_insert_comdat(name)` |
| `LLVMGetComdat` | ❌ | TODO - `gv.comdat` |
| `LLVMSetComdat` | ❌ | TODO - `gv.comdat = comdat` |
| `LLVMGetComdatSelectionKind` | ❌ | TODO - `comdat.selection_kind` |
| `LLVMSetComdatSelectionKind` | ❌ | TODO - `comdat.selection_kind = kind` |

**Proposed API:**

```python
# Proposed (not yet implemented)
comdat = mod.get_or_insert_comdat("my_comdat")
comdat.selection_kind = llvm.ComdatSelectionKind.Any

fn.comdat = comdat
gv.comdat = comdat
```

**Coverage:** 0/5 (0%)

---

## Summary

| Header | Total | Implemented | Coverage |
|--------|-------|-------------|----------|
| Analysis.h | 4 | 1 | 25% |
| BitReader.h | 8 | 3 | 37.5% |
| BitWriter.h | 4 | 0 | 0% |
| IRReader.h | 1 | 1 | 100% |
| Disassembler.h | 6 | 3 | 50% |
| Object.h | 31 | 23 | 74% |
| PassBuilder.h | 15 | 0 | 0% |
| Linker.h | 1 | 0 | 0% |
| Error.h | 7 | 0 | 0%* |
| ErrorHandling.h | 3 | 0 | 0% |
| Support.h | 4 | 0 | 0% |
| Comdat.h | 5 | 0 | 0% |
| **Total** | **89** | **31** | **35%** |

*Error.h functions are not exposed because errors are handled via Python exceptions.

---

## Priority Implementation Notes

### High Priority

1. **BitWriter.h** - Essential for saving modules
   - `LLVMWriteBitcodeToFile`
   - `LLVMWriteBitcodeToMemoryBuffer`

2. **PassBuilder.h** - Essential for optimization
   - `LLVMRunPasses` with optimization levels

3. **Linker.h** - Module composition
   - `LLVMLinkModules2`

### Medium Priority

4. **Analysis.h** - Better diagnostics
   - `LLVMVerifyFunction`

5. **Comdat.h** - Windows/COFF support
   - All COMDAT APIs

### Lower Priority

6. Support.h - Runtime symbol handling
7. ErrorHandling.h - Custom fatal error handling
