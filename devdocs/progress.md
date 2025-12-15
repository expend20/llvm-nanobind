# LLVM-nanobind Project Progress

This document tracks the implementation progress of the Python bindings for the LLVM-C API.

## Status Overview

**Current State:** Core bindings implemented with partial Python test coverage.

- **C++ Golden Master Tests:** 15/15 passing
- **Python Equivalent Tests:** 2/15 implemented (both passing)
- **Core API Coverage:** ~70% of planned features implemented

---

## Completed Features

### Infrastructure

- [x] ValidityToken for lifetime tracking
- [x] NoMoveCopy base class to prevent accidental copies
- [x] Exception hierarchy (LLVMError, LLVMUseAfterFreeError, LLVMInvalidOperationError, LLVMVerificationError)
- [x] Context managers for Context, Module, and Builder
- [x] Golden master test harness with Python comparison

### Context API

- [x] `llvm.create_context()` - context manager for creating contexts
- [x] `llvm.global_context()` - access to global context
- [x] `ctx.discard_value_names` property

### Type Factory Methods (on Context)

- [x] `ctx.void_type()`
- [x] `ctx.int1_type()`, `int8_type()`, `int16_type()`, `int32_type()`, `int64_type()`, `int128_type()`
- [x] `ctx.int_type(bits)` - arbitrary width integers
- [x] `ctx.half_type()`, `float_type()`, `double_type()`
- [x] `ctx.pointer_type(address_space=0)`
- [x] `ctx.array_type(elem_ty, count)`
- [x] `ctx.vector_type(elem_ty, elem_count)`
- [x] `ctx.function_type(ret_ty, param_types, vararg=False)`
- [x] `ctx.struct_type(elem_types, packed=False)`
- [x] `ctx.named_struct_type(name)` with `set_body()`

### Type Wrapper

- [x] `type.kind` property
- [x] `type.to_string()` / `__str__`
- [x] Type predicates: `is_void`, `is_integer`, `is_float`, `is_pointer`, `is_function`, `is_struct`, `is_array`, `is_vector`
- [x] `type.int_width` for integer types

### Module API

- [x] `ctx.create_module(name)` - context manager
- [x] `mod.name` property (get/set)
- [x] `mod.source_filename` property (get/set)
- [x] `mod.data_layout` property (get/set)
- [x] `mod.target_triple` property (get/set)
- [x] `mod.add_function(name, func_ty)`
- [x] `mod.get_function(name)` - returns Optional
- [x] `mod.add_global(ty, name)`
- [x] `mod.get_global(name)` - returns Optional
- [x] `mod.to_string()` / `__str__`
- [x] `mod.verify()` and `mod.get_verification_error()`
- [x] `mod.clone()`

### Function API

- [x] `func.name` property (inherited from Value)
- [x] `func.param_count`
- [x] `func.get_param(index)`
- [x] `func.params` - list of parameters
- [x] `func.linkage` property (get/set)
- [x] `func.calling_conv` property (get/set)
- [x] `func.append_basic_block(name, ctx)`
- [x] `func.entry_block`, `first_basic_block`, `last_basic_block`
- [x] `func.basic_block_count`
- [x] `func.erase()`

### BasicBlock API

- [x] `bb.name` property
- [x] `bb.as_value()`
- [x] `bb.next_block`, `bb.prev_block`
- [x] `bb.terminator`
- [x] `bb.first_instruction`, `bb.last_instruction`

### Value API

- [x] `val.type` property
- [x] `val.name` property (get/set)
- [x] `val.to_string()` / `__str__`
- [x] `val.is_constant`, `is_undef`, `is_poison`
- [x] `val.add_incoming(val, bb)` - for PHI nodes
- [x] `val.add_case(val, bb)` - for switch instructions
- [x] `val.set_initializer(init)` - for globals
- [x] `val.set_constant(is_const)` - for globals
- [x] `val.set_linkage(linkage)` - for globals
- [x] `val.set_alignment(align)` - for globals

### Builder API

#### Positioning
- [x] `builder.position_at_end(bb)`
- [x] `builder.position_before(inst)`
- [x] `builder.insert_block` property

#### Arithmetic Operations
- [x] `add`, `nsw_add`, `nuw_add`
- [x] `sub`, `nsw_sub`
- [x] `mul`, `nsw_mul`
- [x] `sdiv`, `udiv`
- [x] `srem`, `urem`
- [x] `fadd`, `fsub`, `fmul`, `fdiv`, `frem`
- [x] `neg`, `fneg`
- [x] `not_`

#### Bitwise Operations
- [x] `shl`, `lshr`, `ashr`
- [x] `and_`, `or_`, `xor_`

#### Memory Operations
- [x] `alloca(ty, name)`
- [x] `array_alloca(ty, size, name)`
- [x] `load(ty, ptr, name)`
- [x] `store(val, ptr)`
- [x] `gep(ty, ptr, indices, name)`
- [x] `inbounds_gep(ty, ptr, indices, name)`
- [x] `struct_gep(ty, ptr, idx, name)`

#### Comparison Operations
- [x] `icmp(pred, lhs, rhs, name)`
- [x] `fcmp(pred, lhs, rhs, name)`
- [x] `select(cond, then_val, else_val, name)`

#### Cast Operations
- [x] `trunc`, `zext`, `sext`
- [x] `fptrunc`, `fpext`
- [x] `fptosi`, `fptoui`
- [x] `sitofp`, `uitofp`
- [x] `ptrtoint`, `inttoptr`
- [x] `bitcast`

#### Control Flow
- [x] `ret(val)`, `ret_void()`
- [x] `br(dest)`
- [x] `cond_br(cond, then_bb, else_bb)`
- [x] `switch_(val, else_bb, num_cases)`
- [x] `call(func_ty, func, args, name)`
- [x] `unreachable()`
- [x] `phi(ty, name)`

### Constants API

- [x] `llvm.const_int(ty, val, sign_extend=False)`
- [x] `llvm.const_real(ty, val)`
- [x] `llvm.const_null(ty)`
- [x] `llvm.const_all_ones(ty)`
- [x] `llvm.undef(ty)`
- [x] `llvm.poison(ty)`
- [x] `llvm.const_array(elem_ty, vals)`
- [x] `llvm.const_struct(vals, packed, ctx)`
- [x] `llvm.const_vector(vals)`

### Enumerations

- [x] `Linkage` - External, Internal, Private, etc.
- [x] `Visibility` - Default, Hidden, Protected
- [x] `CallConv` - C, Fast, Cold, X86Stdcall, X86Fastcall
- [x] `IntPredicate` - EQ, NE, UGT, UGE, ULT, ULE, SGT, SGE, SLT, SLE
- [x] `RealPredicate` - all ordered/unordered comparisons
- [x] `TypeKind` - Void, Integer, Float, Pointer, etc.

---

## Not Yet Implemented

### Module API
- [ ] `mod.functions` - iterator over functions
- [ ] `mod.globals` - iterator over global variables
- [ ] `mod.write_bitcode(filename)`
- [ ] `mod.write_ir(filename)`

### Function API
- [ ] `func.visibility` property
- [ ] `func.section` property
- [ ] `func.add_attribute(attr)`
- [ ] `func.add_param_attribute(idx, attr)`
- [ ] `func.basic_blocks` - iterator

### BasicBlock API
- [ ] `bb.name` setter
- [ ] `bb.parent` - owning function
- [ ] `bb.instructions` - iterator
- [ ] `bb.move_before(other)`
- [ ] `bb.move_after(other)`
- [ ] `bb.erase()`

### Instruction API
- [ ] `inst.opcode` property
- [ ] `inst.parent` - owning BasicBlock
- [ ] `inst.num_operands`
- [ ] `inst.get_operand(idx)`
- [ ] `inst.set_operand(idx, val)`
- [ ] `inst.next_instruction`, `prev_instruction`
- [ ] `inst.set_metadata(kind, node)`
- [ ] `inst.get_metadata(kind)`
- [ ] `inst.clone()`
- [ ] `inst.erase()`
- [ ] `inst.remove_from_parent()` / `inst.insert_into(builder)`
- [ ] Instruction-specific properties (is_volatile, alignment, etc.)

### Builder API (Advanced)
- [ ] `builder.position_after(inst)`
- [ ] `builder.invoke(func_ty, func, args, normal_dest, unwind_dest, name)`
- [ ] `builder.atomic_load(ptr, ordering)`
- [ ] `builder.atomic_store(val, ptr, ordering)`
- [ ] `builder.atomicrmw(op, ptr, val, ordering)`
- [ ] `builder.cmpxchg(ptr, expected, desired, success_ordering, failure_ordering)`
- [ ] `builder.fence(ordering)`

### Constants API
- [ ] `llvm.const_int_arbitrary(ty, words)` - for >64-bit integers
- [ ] `llvm.const_string(ctx, string, null_terminate=True)`
- [ ] `llvm.const_named_struct(struct_ty, vals)`
- [ ] `llvm.const_gep(elem_ty, base, indices)`
- [ ] `llvm.const_bitcast(val, dest_ty)`

### Enumerations
- [ ] `AtomicOrdering` - NotAtomic, Unordered, Monotonic, Acquire, Release, etc.
- [ ] `AtomicRMWOp` - Xchg, Add, Sub, And, Or, Xor, Max, Min, etc.
- [ ] `Opcode` - instruction opcodes
- [ ] `Attribute` - function/parameter attributes

### Future Enhancements (from plan.md)
- [ ] Debug Info (DIBuilder)
- [ ] Pass Manager (optimization passes)
- [ ] JIT Compilation (ORC JIT)
- [ ] Target Machine (native code generation)
- [ ] Intrinsics
- [ ] Exception Handling (landingpad, personality)
- [ ] Full Metadata support
- [ ] Thread-safe context management

---

## Test Coverage

### C++ Golden Master Tests

| Test | Status | Python Equivalent |
|------|--------|-------------------|
| test_context | PASS | PASS |
| test_module | PASS | Not implemented |
| test_types | PASS | Not implemented |
| test_function | PASS | Not implemented |
| test_basic_block | PASS | Not implemented |
| test_builder_arithmetic | PASS | Not implemented |
| test_builder_memory | PASS | Not implemented |
| test_builder_control_flow | PASS | Not implemented |
| test_builder_casts | PASS | Not implemented |
| test_builder_cmp | PASS | Not implemented |
| test_constants | PASS | Not implemented |
| test_globals | PASS | Not implemented |
| test_phi | PASS | Not implemented |
| test_factorial | PASS | PASS |
| test_struct | PASS | Not implemented |

---

## Next Steps

### High Priority (Complete Python Test Coverage)

These tests should be straightforward to implement since all required bindings exist:

- [ ] Implement `test_module.py` - module properties, clone
- [ ] Implement `test_types.py` - type factory methods
- [ ] Implement `test_function.py` - function creation, params, linkage
- [ ] Implement `test_basic_block.py` - basic block creation, navigation
- [ ] Implement `test_constants.py` - constant creation
- [ ] Implement `test_globals.py` - global variables

### Medium Priority (More Python Tests)

These require the existing builder operations:

- [ ] Implement `test_builder_arithmetic.py`
- [ ] Implement `test_builder_memory.py`
- [ ] Implement `test_builder_control_flow.py`
- [ ] Implement `test_builder_casts.py`
- [ ] Implement `test_builder_cmp.py`
- [ ] Implement `test_phi.py`
- [ ] Implement `test_struct.py`

### Lower Priority (New Bindings Required)

These require implementing new C++ bindings first:

- [ ] Add module iteration (functions, globals)
- [ ] Add BasicBlock iteration and manipulation
- [ ] Add Instruction API
- [ ] Add atomic operations
- [ ] Add file output (write_bitcode, write_ir)
- [ ] Add const_string

---

## How to Run Tests

```bash
# Build everything
cmake --build build

# Run all tests (C++ golden masters + Python comparison)
python3 run_tests.py

# Run a single Python test manually
python3 test_factorial.py

# Compare Python output to C++ golden master
./build/test_factorial > /tmp/cpp.ll
python3 test_factorial.py > /tmp/py.ll
diff /tmp/cpp.ll /tmp/py.ll
```
