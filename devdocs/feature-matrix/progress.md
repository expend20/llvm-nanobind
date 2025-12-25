# Feature Matrix Progress

## Current Status

**Phase:** Implementation Complete - Maintenance Mode

**Overall Coverage:** ~74% of LLVM-C API

All high-priority items are implemented. Remaining items are low-priority, deprecated, or have better alternatives.

---

## Summary Statistics (Updated December 2024)

| Header | Total | ✅ Impl | 🚫 Skip | ❌ TODO | Coverage |
|--------|-------|---------|---------|---------|----------|
| Core.h | 640 | **445** | 44 | 151 | **70%** |
| DebugInfo.h | 99 | ~50 | 0 | ~49 | ~50% |
| Target.h | 22 | **22** | 0 | 0 | **100%** |
| TargetMachine.h | 29 | **14** | 9 | 6 | **79%** |
| Object.h | 31 | 23 | 0 | 8 | 74% |
| Analysis.h | 4 | **2** | 2 | 0 | **100%** |
| BitReader.h | 8 | 3 | 5 | 0 | 37.5% |
| BitWriter.h | 4 | **2** | 2 | 0 | **100%** |
| IRReader.h | 1 | 1 | 0 | 0 | 100% |
| PassBuilder.h | 15 | **15** | 0 | 0 | **100%** |
| Disassembler.h | 6 | **4** | 0 | 2 | **67%** |
| Linker.h | 1 | **1** | 0 | 0 | **100%** |
| Misc | 20 | 0 | 7 | 13 | 0% |
| **Total** | **~880** | **~582** | **~69** | **~229** | **~74%** |

---

## Recently Implemented (December 2024)

### Session 1 - Core Workflows

#### BitWriter.h - COMPLETE ✅
- `LLVMWriteBitcodeToFile` → `mod.write_bitcode_to_file(path)`
- `LLVMWriteBitcodeToMemoryBuffer` → `mod.write_bitcode_to_memory_buffer()`

#### PassBuilder.h - COMPLETE ✅
All 15 functions implemented including all optimization options.

#### Target.h - COMPLETE ✅
All 22 functions including host queries, initialization, and target data.

#### TargetMachine.h - 79%
Core code generation fully working. Only TargetMachineOptions builder API missing (not needed).

#### Linker.h - COMPLETE ✅
- `LLVMLinkModules2` → `mod.link_module(src_mod)`

#### Analysis.h - COMPLETE ✅
- Module and function verification implemented.

### Session 2 - Additional APIs

#### Module Flags
- `LLVMAddModuleFlag` → `mod.add_module_flag(behavior, key, val)`
- `LLVMGetModuleFlag` → `mod.get_module_flag(key)`
- `LLVMModuleFlagBehavior` enum → `llvm.ModuleFlagBehavior`

#### String Attributes
- `LLVMCreateStringAttribute` → `ctx.create_string_attribute(key, val)`
- `LLVMGetStringAttributeKind` → `attr.string_kind`
- `LLVMGetStringAttributeValue` → `attr.string_value`
- `LLVMIsEnumAttribute` → `attr.is_enum_attribute`
- `LLVMIsStringAttribute` → `attr.is_string_attribute`
- `LLVMIsTypeAttribute` → `attr.is_type_attribute`

#### Type Queries
- `LLVMIsLiteralStruct` → `ty.is_literal_struct`

#### Value Manipulation
- `LLVMSetOperand` → `val.set_operand(index, new_val)`

#### Constant Creation from Strings
- `LLVMConstIntOfStringAndSize` → `ty.constant_from_string(text, radix)`
- `LLVMConstRealOfStringAndSize` → `ty.real_constant_from_string(text)`

#### Disassembler Options
- `LLVMSetDisasmOptions` → `disasm.set_options(options)`
- Disassembler option constants (`DisasmOption_*`)

#### Builder Instructions
- `LLVMBuildAddrSpaceCast` → `b.addr_space_cast(val, ty, name)`
- `LLVMBuildFence` → `b.fence(ordering, single_thread, name)`

---

## Remaining TODO - Detailed Breakdown

### Medium Priority - Could Be Useful

#### Type Attributes (Core.h)
| Function | Description | Proposed API |
|----------|-------------|--------------|
| `LLVMCreateTypeAttribute` | Create attribute with type value | `ctx.create_type_attribute(kind, ty)` |
| `LLVMGetTypeAttributeValue` | Get type from type attribute | `attr.type_value` |
| `LLVMCreateConstantRangeAttribute` | Create range attribute | `ctx.create_range_attribute(kind, bits, lo, hi)` |

#### Module Flag Iteration (Core.h)
| Function | Description | Proposed API |
|----------|-------------|--------------|
| `LLVMCopyModuleFlagsMetadata` | Get all module flags | `mod.get_all_module_flags()` |
| `LLVMDisposeModuleFlagsMetadata` | Free flag entries | Automatic |
| `LLVMModuleFlagEntriesGetFlagBehavior` | Get flag behavior | Via iterator |
| `LLVMModuleFlagEntriesGetKey` | Get flag key | Via iterator |
| `LLVMModuleFlagEntriesGetMetadata` | Get flag metadata | Via iterator |

#### Attribute Management (Core.h)
| Function | Description | Proposed API |
|----------|-------------|--------------|
| `LLVMGetAttributesAtIndex` | Get all attributes at index | `fn.get_attributes(index)` |
| `LLVMGetStringAttributeAtIndex` | Get string attribute | `fn.get_string_attribute(index, key)` |
| `LLVMRemoveEnumAttributeAtIndex` | Remove enum attribute | `fn.remove_attribute(index, kind)` |
| `LLVMRemoveStringAttributeAtIndex` | Remove string attribute | `fn.remove_string_attribute(index, key)` |
| `LLVMAddTargetDependentFunctionAttr` | Add target-specific attr | `fn.add_target_attribute(key, val)` |

#### Builder Position Control (Core.h)
| Function | Description | Proposed API |
|----------|-------------|--------------|
| `LLVMPositionBuilder` | Position at instruction | `b.position_at(bb, inst)` |
| `LLVMClearInsertionPosition` | Clear position | `b.clear_position()` |
| `LLVMBuilderGetDefaultFPMathTag` | Get FP math tag | `b.default_fp_math_tag` |
| `LLVMBuilderSetDefaultFPMathTag` | Set FP math tag | `b.default_fp_math_tag = tag` |

#### Additional Builder Instructions (Core.h)
| Function | Description | Proposed API |
|----------|-------------|--------------|
| `LLVMBuildExactUDiv` | Exact unsigned division | `b.exact_udiv(lhs, rhs, name)` |
| `LLVMBuildNUWNeg` | No unsigned wrap negation | `b.nuw_neg(val, name)` |
| `LLVMBuildIndirectBr` | Indirect branch | `b.indirect_br(addr, num_dests)` |
| `LLVMAddDestination` | Add indirect branch dest | `indirect_br.add_destination(bb)` |
| `LLVMBuildAtomicRMW` | Atomic RMW (no sync scope) | Use `b.atomic_rmw_sync_scope()` |
| `LLVMBuildAtomicCmpXchg` | Atomic cmpxchg (no sync scope) | Use `b.atomic_cmpxchg_sync_scope()` |

#### Convenience Cast Builders (Core.h)
| Function | Description | Proposed API |
|----------|-------------|--------------|
| `LLVMBuildZExtOrBitCast` | Zero extend or bitcast | `b.zext_or_bitcast(val, ty, name)` |
| `LLVMBuildSExtOrBitCast` | Sign extend or bitcast | `b.sext_or_bitcast(val, ty, name)` |
| `LLVMBuildTruncOrBitCast` | Truncate or bitcast | `b.trunc_or_bitcast(val, ty, name)` |
| `LLVMBuildCast` | Generic cast | `b.cast(op, val, ty, name)` |
| `LLVMBuildPointerCast` | Pointer cast | `b.pointer_cast(val, ty, name)` |
| `LLVMBuildFPCast` | FP cast | `b.fp_cast(val, ty, name)` |
| `LLVMGetCastOpcode` | Get cast opcode | `llvm.get_cast_opcode(src, dst, ...)` |

#### Value/Use Access (Core.h)
| Function | Description | Proposed API |
|----------|-------------|--------------|
| `LLVMGetOperandUse` | Get use at operand index | `val.get_operand_use(index)` |
| `LLVMBlockAddress` | Get block address constant | `llvm.block_address(fn, bb)` |

#### Intrinsics (Core.h)
| Function | Description | Proposed API |
|----------|-------------|--------------|
| `LLVMIntrinsicGetType` | Get intrinsic function type | `llvm.intrinsic_get_type(ctx, id, params)` |

#### Memory Buffer (Core.h)
| Function | Description | Proposed API |
|----------|-------------|--------------|
| `LLVMCreateMemoryBufferWithSTDIN` | Read from stdin | `llvm.MemoryBuffer.from_stdin()` |
| `LLVMCreateMemoryBufferWithMemoryRange` | From memory range | `llvm.MemoryBuffer.from_bytes(data, name, copy)` |

#### Metadata (Core.h)
| Function | Description | Proposed API |
|----------|-------------|--------------|
| `LLVMReplaceMDNodeOperandWith` | Replace MD operand | `md.replace_operand(index, new_md)` |
| `LLVMGetMDKindIDInContext` | Get metadata kind ID | `ctx.get_md_kind_id(name)` |

---

### Low Priority - Rarely Needed

#### Global IFunc (Core.h)
| Function | Description | Notes |
|----------|-------------|-------|
| `LLVMEraseGlobalIFunc` | Erase IFunc | Rare use case |
| `LLVMRemoveGlobalIFunc` | Remove IFunc | Rare use case |

#### DLL Storage Class (Core.h)
| Function | Description | Notes |
|----------|-------------|-------|
| `LLVMGetDLLStorageClass` | Get DLL storage | Windows-specific |
| `LLVMSetDLLStorageClass` | Set DLL storage | Windows-specific |

#### Sync Scope (Core.h)
| Function | Description | Notes |
|----------|-------------|-------|
| `LLVMGetSyncScopeID` | Get sync scope ID | Advanced atomics |

#### X86-Specific Types (Core.h)
| Function | Description | Notes |
|----------|-------------|-------|
| `LLVMX86MMXTypeInContext` | X86 MMX type | x86-specific, legacy |
| `LLVMX86AMXTypeInContext` | X86 AMX type | x86-specific |

#### Arbitrary Precision Constants (Core.h)
| Function | Description | Notes |
|----------|-------------|-------|
| `LLVMConstIntOfArbitraryPrecision` | Arbitrary precision int | Use `constant_from_string` |
| `LLVMConstVector` | Create constant vector | Can use array + bitcast |

---

### Intentionally Skipped (🚫)

#### Deprecated Functions
| Function | Replacement |
|----------|-------------|
| `LLVMArrayType` | Use `LLVMArrayType2` (already bound) |
| `LLVMConstArray` | Use `LLVMConstArray2` (already bound) |
| `LLVMWriteBitcodeToFileHandle` | Use `LLVMWriteBitcodeToFile` |
| `LLVMInsertIntoBuilder` | Use specific build methods |

#### Const Expression Builders
These are deprecated - use builder instructions instead:
| Function | Alternative |
|----------|-------------|
| `LLVMConstNeg`, `LLVMConstNot`, etc. | Build in function |
| `LLVMConstAdd`, `LLVMConstSub`, etc. | Build in function |
| `LLVMConstGEP2`, `LLVMConstInBoundsGEP2` | Use `builder.gep()` |
| `LLVMConstTrunc`, `LLVMConstZExt`, etc. | Use builder casts |
| `LLVMConstBitCast`, `LLVMConstAddrSpaceCast` | Use builder casts |
| `LLVMConstICmp`, `LLVMConstFCmp` | Use `builder.icmp/fcmp` |
| `LLVMConstSelect` | Use `builder.select` |

#### Dump Functions
Use Python's `print()` instead:
| Function | Alternative |
|----------|-------------|
| `LLVMDumpModule` | `print(mod)` |
| `LLVMDumpType` | `print(ty)` |
| `LLVMDumpValue` | `print(val)` |

#### Redundant Functions
| Function | Alternative |
|----------|-------------|
| `LLVMGetNamedFunctionWithLength` | Use `mod.get_function(name)` |
| `LLVMAppendModuleInlineAsm` | Use `mod.inline_asm += "..."` |
| `LLVMGetLastEnumAttributeKind` | Not useful in Python |

#### Debugging-Only Functions
| Function | Notes |
|----------|-------|
| `LLVMViewFunctionCFG` | Requires graphviz, debugging only |
| `LLVMViewFunctionCFGOnly` | Requires graphviz, debugging only |

#### Low-Level File APIs
| Function | Notes |
|----------|-------|
| `LLVMWriteBitcodeToFD` | Use file path or memory buffer |

---

### Advanced Features - Not Yet Implemented

#### Comdat.h (Windows/COFF Linking)
| Function | Description | Priority |
|----------|-------------|----------|
| `LLVMGetOrInsertComdat` | Get/create COMDAT | Low |
| `LLVMGetComdat` | Get global's COMDAT | Low |
| `LLVMSetComdat` | Set global's COMDAT | Low |
| `LLVMGetComdatSelectionKind` | Get selection kind | Low |
| `LLVMSetComdatSelectionKind` | Set selection kind | Low |

**Use Case:** COMDAT sections for deduplication in Windows COFF object files.

#### ErrorHandling.h (Fatal Error Handling)
| Function | Description | Priority |
|----------|-------------|----------|
| `LLVMInstallFatalErrorHandler` | Install custom handler | Low |
| `LLVMResetFatalErrorHandler` | Reset to default | Low |
| `LLVMEnablePrettyStackTrace` | Enable stack traces | Low |

**Use Case:** Custom handling of LLVM fatal errors (normally crashes).

#### Support.h (Runtime Symbol Resolution)
| Function | Description | Priority |
|----------|-------------|----------|
| `LLVMLoadLibraryPermanently` | Load shared library | Low |
| `LLVMParseCommandLineOptions` | Parse LLVM cl options | Low |
| `LLVMSearchForAddressOfSymbol` | Find symbol address | Low |
| `LLVMAddSymbol` | Add symbol to table | Low |

**Use Case:** JIT symbol resolution and dynamic library loading.

#### TargetMachineOptions (Builder API)
| Function | Description | Priority |
|----------|-------------|----------|
| `LLVMCreateTargetMachineOptions` | Create options builder | Low |
| `LLVMDisposeTargetMachineOptions` | Dispose options | Low |
| `LLVMTargetMachineOptionsSetCPU` | Set CPU | Low |
| `LLVMTargetMachineOptionsSetFeatures` | Set features | Low |
| `LLVMTargetMachineOptionsSetABI` | Set ABI | Low |
| `LLVMTargetMachineOptionsSetRelocationModel` | Set reloc model | Low |
| `LLVMTargetMachineOptionsSetCodeGenOptLevel` | Set opt level | Low |
| `LLVMCreateTargetMachineWithOptions` | Create TM with options | Low |

**Use Case:** Alternative builder pattern for target machine creation. Current `create_target_machine()` is sufficient.

#### ORC JIT APIs (Orc.h, LLJIT.h)
Not yet tracked in detail. These provide modern JIT compilation:
- ORC lazy compilation
- LLJIT high-level interface
- Object linking layer
- Symbol resolution

**Use Case:** JIT compilation. Consider implementing if JIT is needed.

#### DebugInfo.h Remaining (~49 functions)
Many DI creation functions for various DWARF constructs:
- Composite types (arrays, vectors)
- Derived types
- Subranges
- Template parameters
- Imported entities
- etc.

**Use Case:** Complete debug info for all language constructs.

---

## Implementation Notes

### Core Workflows - All Supported ✅

1. **IR Creation** - Full support for modules, functions, types, instructions
2. **Bitcode I/O** - Read and write bitcode files
3. **Optimization** - Full PassBuilder support with all options
4. **Code Generation** - Emit object files and assembly
5. **Module Linking** - Link modules together
6. **Verification** - Verify modules and functions
7. **Debug Info** - Core DIBuilder support

### Safety Model

All bindings include:
- Lifetime tracking via `ValidityToken`
- Null checks before LLVM-C calls
- Python exceptions instead of crashes
- Automatic resource cleanup

### Testing

- Golden master tests comparing C++ and Python output
- Memory safety tests
- Type checking with `ty check`

---

## Changelog

### December 2024
- Implemented all high-priority items (BitWriter, PassBuilder, Target, Linker)
- Added module flags, string attributes, constant from string
- Added builder instructions (addr_space_cast, fence)
- Added disassembler options
- Updated documentation with detailed remaining items
- Overall coverage: 74%
