# Porting C++ LLVM Code to Python Bindings

This guide documents the experience of porting the LLVM Obfuscator passes from C++ to Python. It covers API differences, missing functionality, confusing patterns, and workarounds discovered during the port.

## Table of Contents

1. [Context and Module Management](#context-and-module-management)
2. [Type System](#type-system)
3. [Instructions and Values](#instructions-and-values)
4. [Basic Blocks](#basic-blocks)
5. [Builder API](#builder-api)
6. [Control Flow](#control-flow)
7. [PHI Nodes](#phi-nodes)
8. [Global Variables](#global-variables)
9. [Constants](#constants)
10. [Memory Management](#memory-management)
11. [Missing APIs](#missing-apis)
12. [API Naming Differences](#api-naming-differences)
13. [Common Pitfalls](#common-pitfalls)

---

## Context and Module Management

### Parsing IR

**C++ (LLVM API):**
```cpp
LLVMContext ctx;
SMDiagnostic err;
std::unique_ptr<Module> mod = parseIRFile("input.ll", err, ctx);
```

**Python:**
```python
with llvm.create_context() as ctx:
    with ctx.parse_ir(ir_text) as mod:
        # use mod here
```

**Key Differences:**
- `ctx.parse_ir()` returns a **context manager** (`ModuleManager`), not a `Module` directly
- You MUST use `with` statement to get the actual `Module` object
- The module is automatically disposed when exiting the `with` block

**Common Mistake:**
```python
# WRONG - mod is a ModuleManager, not a Module!
mod = ctx.parse_ir(ir_text)
for func in mod.functions:  # AttributeError!

# CORRECT
with ctx.parse_ir(ir_text) as mod:
    for func in mod.functions:  # Works
```

### Context Lifetime Warning

If a module outlives its context, you'll see:
```
Warning: LLVM Module outlived its Context. This may cause a memory leak.
```

Always ensure modules are disposed before their context.

---

## Type System

### Accessing Types

**C++ (IRBuilder):**
```cpp
IRBuilder<> builder(ctx);
Type* i32 = builder.getInt32Ty();
Type* i64 = builder.getInt64Ty();
Type* ptr = builder.getPtrTy();
```

**Python:**
```python
i32 = ctx.types.i32      # Property, not method
i64 = ctx.types.i64      # Property, not method  
ptr = ctx.types.ptr()    # METHOD - must call!
```

**Critical Difference:** Most types are properties (`i32`, `i64`, `i8`, `f32`, etc.) but `ptr()` is a METHOD that must be called:

```python
# WRONG
ptr_ty = ctx.types.ptr   # This is a bound method object!
array_ty = ctx.types.array(ptr_ty, 2)  # TypeError!

# CORRECT
ptr_ty = ctx.types.ptr()  # Call the method
array_ty = ctx.types.array(ptr_ty, 2)  # Works
```

### Creating Array Types

**C++:**
```cpp
ArrayType* arrTy = ArrayType::get(elemTy, count);
```

**Python:**
```python
arr_ty = ctx.types.array(elem_ty, count)
```

### Creating Function Types

**C++:**
```cpp
FunctionType* fnTy = FunctionType::get(retTy, {paramTy1, paramTy2}, false);
```

**Python:**
```python
fn_ty = ctx.types.function(ret_ty, [param_ty1, param_ty2])
```

### Getting Integer Bit Width

**C++:**
```cpp
unsigned width = intTy->getIntegerBitWidth();
```

**Python:**
```python
width = int_ty.int_width  # Not .width or .bit_width
```

---

## Instructions and Values

### Instruction Type

In Python, all instructions are `llvm.Value` objects. There is no separate `Instruction` class:

```python
for inst in bb.instructions:
    print(type(inst))  # <class 'llvm.Value'>
```

### Getting Instruction Opcode

**C++:**
```cpp
if (inst->getOpcode() == Instruction::Add) { ... }
```

**Python:**
```python
if inst.opcode == llvm.Opcode.Add:  # Note: llvm.Opcode enum
```

Available opcodes include: `Add`, `Sub`, `Mul`, `Xor`, `Or`, `And`, `Br`, `PHI`, etc.

### Getting Operands

**C++:**
```cpp
Value* op0 = inst->getOperand(0);
Value* op1 = inst->getOperand(1);
unsigned numOps = inst->getNumOperands();
```

**Python:**
```python
op0 = inst.get_operand(0)  # Method, not property
op1 = inst.get_operand(1)
num_ops = inst.num_operands  # Property
```

**Note:** There is NO `.operands` iterator property. You must use index-based access:

```python
# WRONG
for op in inst.operands:  # AttributeError!

# CORRECT
for i in range(inst.num_operands):
    op = inst.get_operand(i)
```

### Setting Operands

**C++:**
```cpp
inst->setOperand(0, newValue);
```

**Python:**
```python
inst.set_operand(0, new_value)
```

### Checking Instruction Properties

**C++:**
```cpp
if (inst->isTerminator()) { ... }
```

**Python:**
```python
if inst.is_terminator_inst:  # Note the suffix
```

Other properties:
- `inst.is_conditional` - for branch instructions
- `inst.opcode` - get opcode enum

### Getting Parent Block

**C++:**
```cpp
BasicBlock* bb = inst->getParent();
```

**Python:**
```python
bb = inst.block  # NOT .parent!
```

---

## Basic Blocks

### Iterating Over Blocks

**C++:**
```cpp
for (BasicBlock& bb : func) { ... }
```

**Python:**
```python
for bb in func.basic_blocks:  # Property returning iterator
```

### Getting Instructions

**C++:**
```cpp
for (Instruction& inst : bb) { ... }
```

**Python:**
```python
for inst in bb.instructions:  # Property returning iterator
```

### Getting Terminator

**C++:**
```cpp
Instruction* term = bb.getTerminator();
```

**Python:**
```python
term = bb.terminator  # Property
```

### Creating Basic Blocks

**C++:**
```cpp
BasicBlock* bb = BasicBlock::Create(ctx, "name", func);
```

**Python:**
```python
bb = func.append_basic_block("name")
```

### Moving Blocks

**C++:**
```cpp
bb->moveAfter(otherBB);
bb->moveBefore(otherBB);
```

**Python:**
```python
bb.move_after(other_bb)
bb.move_before(other_bb)
```

### Splitting Blocks

**MISSING API:** There is no `splitBasicBlock()` or `splitBefore()` method in the Python bindings. You cannot split a basic block at an instruction.

**Workaround:** Create a new block and manually move instructions (complex, not fully supported).

---

## Builder API

### Creating a Builder

**C++:**
```cpp
IRBuilder<> builder(bb);
// or
IRBuilder<> builder(insertPoint);
```

**Python:**
```python
with bb.create_builder() as builder:
    # use builder here
```

The builder is a context manager and should be used with `with`.

### Positioning the Builder

**C++:**
```cpp
builder.SetInsertPoint(bb);
builder.SetInsertPoint(inst);
```

**Python:**
```python
builder.position_at_end(bb)
builder.position_before(inst)
builder.position_at(bb, inst)  # Position at specific point
```

**Note:** `SetInsertPoint(inst)` maps to `position_before(inst)` in Python.

### Getting Current Insert Block

**C++:**
```cpp
BasicBlock* bb = builder.GetInsertBlock();
```

**Python:**
```python
bb = builder.get_insert_block()
```

### Arithmetic Instructions

| C++ | Python |
|-----|--------|
| `builder.CreateAdd(a, b, "name")` | `builder.add(a, b, "name")` |
| `builder.CreateSub(a, b, "name")` | `builder.sub(a, b, "name")` |
| `builder.CreateMul(a, b, "name")` | `builder.mul(a, b, "name")` |
| `builder.CreateNeg(a, "name")` | `builder.neg(a, "name")` |
| `builder.CreateNot(a, "name")` | `builder.not_(a, "name")` |
| `builder.CreateAnd(a, b, "name")` | `builder.and_(a, b, "name")` |
| `builder.CreateOr(a, b, "name")` | `builder.or_(a, b, "name")` |
| `builder.CreateXor(a, b, "name")` | `builder.xor(a, b, "name")` |

**Note:** `not_` and `and_` have trailing underscores to avoid Python keyword conflicts.

### Memory Instructions

| C++ | Python |
|-----|--------|
| `builder.CreateAlloca(ty, nullptr, "name")` | `builder.alloca(ty, name="name")` |
| `builder.CreateLoad(ty, ptr, "name")` | `builder.load(ty, ptr, "name")` |
| `builder.CreateStore(val, ptr)` | `builder.store(val, ptr)` |
| `builder.CreateGEP(ty, ptr, indices, "name")` | `builder.gep(ty, ptr, indices, "name")` |

### Control Flow Instructions

| C++ | Python |
|-----|--------|
| `builder.CreateBr(destBB)` | `builder.br(dest_bb)` |
| `builder.CreateCondBr(cond, trueBB, falseBB)` | `builder.cond_br(cond, true_bb, false_bb)` |
| `builder.CreateRet(val)` | `builder.ret(val)` |
| `builder.CreateRetVoid()` | `builder.ret_void()` |
| `builder.CreateIndirectBr(addr, numDests)` | `builder.indirect_br(addr, num_dests)` |

### Comparison Instructions

**C++:**
```cpp
Value* cmp = builder.CreateICmpEQ(a, b, "cmp");
Value* cmp = builder.CreateICmpSLT(a, b, "cmp");
```

**Python:**
```python
cmp = builder.icmp(llvm.IntPredicate.EQ, a, b, "cmp")
cmp = builder.icmp(llvm.IntPredicate.SLT, a, b, "cmp")
```

Available predicates: `EQ`, `NE`, `UGT`, `UGE`, `ULT`, `ULE`, `SGT`, `SGE`, `SLT`, `SLE`

### Type Conversions

| C++ | Python |
|-----|--------|
| `builder.CreateZExt(val, destTy, "name")` | `builder.zext(val, dest_ty, "name")` |
| `builder.CreateSExt(val, destTy, "name")` | `builder.sext(val, dest_ty, "name")` |
| `builder.CreateTrunc(val, destTy, "name")` | `builder.trunc(val, dest_ty, "name")` |

---

## Control Flow

### Branch Instruction Properties

**C++:**
```cpp
BranchInst* br = dyn_cast<BranchInst>(term);
if (br->isConditional()) {
    Value* cond = br->getCondition();
    BasicBlock* trueDest = br->getSuccessor(0);
    BasicBlock* falseDest = br->getSuccessor(1);
}
```

**Python:**
```python
if term.is_conditional:
    cond = term.condition
    successors = list(term.successors)
    true_dest = successors[0]
    false_dest = successors[1]
```

### Indirect Branch

**C++:**
```cpp
IndirectBrInst* ibr = builder.CreateIndirectBr(addr, numDests);
ibr->addDestination(bb1);
ibr->addDestination(bb2);
```

**Python:**
```python
ibr = builder.indirect_br(addr, num_dests)
ibr.add_destination(bb1)
ibr.add_destination(bb2)
```

### Block Address

**C++:**
```cpp
BlockAddress* addr = BlockAddress::get(func, bb);
```

**Python:**
```python
addr = llvm.block_address(func, bb)  # Function, not class method
```

---

## PHI Nodes

### Getting PHI Information

**C++:**
```cpp
PHINode* phi = dyn_cast<PHINode>(inst);
unsigned numIncoming = phi->getNumIncomingValues();
Value* val = phi->getIncomingValue(i);
BasicBlock* bb = phi->getIncomingBlock(i);
```

**Python:**
```python
num_incoming = phi.num_incoming
val = phi.get_incoming_value(i)
bb = phi.get_incoming_block(i)
```

### Creating PHI Nodes

**C++:**
```cpp
PHINode* phi = builder.CreatePHI(type, numReserved, "name");
phi->addIncoming(val1, bb1);
phi->addIncoming(val2, bb2);
```

**Python:**
```python
phi = builder.phi(type, "name")
phi.add_incoming(val1, bb1)
phi.add_incoming(val2, bb2)
```

---

## Global Variables

### Iterating Over Globals

**C++:**
```cpp
for (GlobalVariable& gv : module.globals()) { ... }
```

**Python:**
```python
for gv in mod.globals:  # Property
```

### Checking for Initializer

**C++:**
```cpp
if (gv.hasInitializer()) {
    Constant* init = gv.getInitializer();
}
```

**Python:**
```python
init = gv.initializer  # Returns None if no initializer
if init is not None:
    # use init
```

**Note:** There is no `has_initializer` property. Check if `initializer` is `None`.

### Setting Initializer

**C++:**
```cpp
gv.setInitializer(newInit);
```

**Python:**
```python
gv.initializer = new_init
```

**CRITICAL:** The new initializer's type MUST exactly match the global's type. LLVM will assert/crash otherwise.

### Setting Global Properties

**C++:**
```cpp
gv.setConstant(false);
gv.setLinkage(GlobalValue::InternalLinkage);
```

**Python:**
```python
gv.set_constant(False)  # Method, not property setter
gv.linkage = llvm.Linkage.Internal  # Property setter
```

### Creating Global Variables

**C++:**
```cpp
GlobalVariable* gv = new GlobalVariable(
    module, type, isConstant, linkage, initializer, name);
```

**Python:**
```python
gv = mod.add_global(type, name)
gv.initializer = initializer
gv.linkage = llvm.Linkage.Internal
```

---

## Constants

### Integer Constants

**C++:**
```cpp
ConstantInt* c = ConstantInt::get(i32Ty, 42);
ConstantInt* c = ConstantInt::getSigned(i32Ty, -1);
```

**Python:**
```python
c = i32_ty.constant(42)
c = i32_ty.constant(-1)  # Signed by default
c = i32_ty.constant(value, sign_extend=True)  # Explicit
```

**Integer Range Limitation:** The `constant()` method may fail for values outside the signed 64-bit range. For large unsigned values, you may need to use signed representation.

### Null Pointer Constant

**C++:**
```cpp
Constant* null = ConstantPointerNull::get(ptrTy);
```

**Python:**
```python
null = llvm.ConstantPointerNull.get(ptr_ty)
```

### String Constants

**C++:**
```cpp
Constant* str = ConstantDataArray::getString(ctx, "hello", true);
```

**Python:**
```python
str_const = llvm.const_string(ctx, "hello", dont_null_terminate=False)
```

**CRITICAL BUG:** String constants are encoded as UTF-8 internally. If your string contains bytes > 127, the resulting array will be larger than expected:

```python
# String with high bytes
data = "Hello\xff\x80"  # 7 characters
const = llvm.const_string(ctx, data, dont_null_terminate=True)
# const.type might be [10 x i8] instead of [7 x i8]!
```

This makes string encryption impossible when encrypted bytes exceed ASCII range.

### Array Constants

**C++:**
```cpp
Constant* arr = ConstantArray::get(arrTy, {elem1, elem2});
Constant* dataArr = ConstantDataArray::get(ctx, arrayRef);
```

**Python:**
```python
arr = llvm.const_array(elem_ty, [elem1, elem2])
data_arr = llvm.const_data_array(elem_ty, string_data)
```

**Same UTF-8 bug applies to `const_data_array`.**

### Struct Constants

**C++:**
```cpp
Constant* s = ConstantStruct::get(structTy, {field1, field2});
```

**Python:**
```python
s = llvm.const_named_struct(struct_ty, [field1, field2])
```

---

## Memory Management

### Replacing All Uses

**C++:**
```cpp
oldValue->replaceAllUsesWith(newValue);
```

**Python:** There is NO `replace_all_uses_with()` method on `Value`. You must implement it manually:

```python
def replace_all_uses_with(old_value, new_value):
    """Replace all uses of old_value with new_value."""
    uses_to_replace = []
    for use in old_value.uses:
        uses_to_replace.append(use.user)
    
    for user in uses_to_replace:
        for i in range(user.num_operands):
            if user.get_operand(i) == old_value:
                user.set_operand(i, new_value)
```

### Deleting Instructions

**C++:**
```cpp
inst->eraseFromParent();
```

**Python:** Two-step process required:

```python
inst.remove_from_parent()  # Unlink from block
inst.delete_instruction()  # Actually delete
```

If you call `delete_instruction()` without `remove_from_parent()` first, LLVM will assert:
```
Assertion failed: (!getParent() && "Instruction still linked in the program!")
```

### Instruction Uses

**C++:**
```cpp
for (Use& use : inst->uses()) {
    User* user = use.getUser();
}
```

**Python:**
```python
for use in inst.uses:
    user = use.user
    # use.used_value gives the value being used
```

---

## Missing APIs

The following LLVM C++ APIs have no Python equivalent:

### Basic Block Operations
- `BasicBlock::splitBasicBlock()` - Split block at instruction
- `BasicBlock::splitBasicBlockBefore()` - Split before instruction

### Instruction Operations
- `Instruction::insertBefore()` - Insert instruction before another
- `Instruction::insertAfter()` - Insert instruction after another
- `Instruction::moveBefore()` - Move instruction
- `Instruction::moveAfter()` - Move instruction
- `Instruction::clone()` - Clone instruction

### Value Operations
- `Value::replaceAllUsesWith()` - Replace all uses (must implement manually)

### Module Operations
- `Module::materializeAll()` - Materialize lazy module

### Type Checking
- `isa<T>()`, `dyn_cast<T>()` - No type casting, use `.opcode` or `.is_*` properties

### Function Operations
- `Function::viewCFG()` - View control flow graph
- `Function::viewCFGOnly()` - View CFG without instructions

---

## API Naming Differences

### General Patterns

| C++ Pattern | Python Pattern |
|-------------|----------------|
| `getFoo()` | `foo` (property) or `get_foo()` |
| `setFoo(x)` | `foo = x` (property) or `set_foo(x)` |
| `hasFoo()` | Check if property is `None` |
| `isFoo()` | `is_foo` (property) |
| `getNumFoo()` | `num_foo` (property) |
| `CreateFoo()` | `foo()` (on builder) |

### Specific Renames

| C++ | Python |
|-----|--------|
| `getParent()` | `.block` (for instructions) |
| `getTerminator()` | `.terminator` |
| `getNumOperands()` | `.num_operands` |
| `getOperand(i)` | `.get_operand(i)` |
| `isTerminator()` | `.is_terminator_inst` |
| `getIntegerBitWidth()` | `.int_width` |
| `eraseFromParent()` | `.remove_from_parent()` + `.delete_instruction()` |

---

## Common Pitfalls

### 1. Forgetting Context Manager for Modules

```python
# WRONG
mod = ctx.parse_ir(ir_text)
mod.functions  # AttributeError

# CORRECT
with ctx.parse_ir(ir_text) as mod:
    mod.functions  # Works
```

### 2. Forgetting to Call `ptr()`

```python
# WRONG
ptr_ty = ctx.types.ptr  # Bound method!

# CORRECT  
ptr_ty = ctx.types.ptr()  # Type object
```

### 3. Using Wrong Property for Parent Block

```python
# WRONG
bb = inst.parent  # AttributeError

# CORRECT
bb = inst.block
```

### 4. Trying to Iterate Operands

```python
# WRONG
for op in inst.operands:  # No such property

# CORRECT
for i in range(inst.num_operands):
    op = inst.get_operand(i)
```

### 5. Forgetting Two-Step Deletion

```python
# WRONG - will crash
inst.delete_instruction()

# CORRECT
inst.remove_from_parent()
inst.delete_instruction()
```

### 6. Integer Overflow in Constants

```python
# May fail for large values
val = i64_ty.constant(0xFFFFFFFFFFFFFFFF)  # Too large!

# Use signed representation
val = i64_ty.constant(-1)  # Same bit pattern, works
```

### 7. String Constants with High Bytes

```python
# Encrypted string with bytes > 127
encrypted = bytes([0xFF, 0x80, 0x42]).decode('latin-1')
const = llvm.const_string(ctx, encrypted, True)
# Type will be wrong due to UTF-8 encoding!
```

### 8. Exception Types

```python
# WRONG
except llvm.LLVMException:  # No such exception

# CORRECT
except llvm.LLVMError:  # Base exception class
```

Available exceptions: `LLVMError`, `LLVMParseError`, `LLVMAssertionError`, `LLVMMemoryError`

---

## Recommended Patterns

### Safe Instruction Replacement

```python
def replace_instruction(old_inst, new_value):
    """Safely replace an instruction with a new value."""
    # Replace all uses
    for use in list(old_inst.uses):
        user = use.user
        for i in range(user.num_operands):
            if user.get_operand(i) == old_inst:
                user.set_operand(i, new_value)
    
    # Delete old instruction
    old_inst.remove_from_parent()
    old_inst.delete_instruction()
```

### Safe Iteration with Modification

```python
# When modifying instructions while iterating, collect first
to_process = []
for bb in func.basic_blocks:
    for inst in bb.instructions:
        if should_process(inst):
            to_process.append(inst)

# Then modify
for inst in to_process:
    transform(inst)
```

### Getting All Instructions of a Type

```python
def get_instructions_by_opcode(func, opcode):
    """Get all instructions with given opcode."""
    result = []
    for bb in func.basic_blocks:
        for inst in bb.instructions:
            if inst.opcode == opcode:
                result.append(inst)
    return result

# Usage
branches = get_instructions_by_opcode(func, llvm.Opcode.Br)
phis = get_instructions_by_opcode(func, llvm.Opcode.PHI)
```

---

## Additional Observations

### No Type Discrimination

There's no way to check if a Value is a specific instruction type like in C++:

**C++:**
```cpp
if (auto* br = dyn_cast<BranchInst>(inst)) {
    // use br
}
if (isa<PHINode>(inst)) {
    // it's a PHI
}
```

**Python:** Must use opcode checking:
```python
if inst.opcode == llvm.Opcode.Br:
    # it's a branch
if inst.opcode == llvm.Opcode.PHI:
    # it's a PHI
```

This works but is less elegant than proper type discrimination.

### Value Equality

Comparing values works correctly with `==`:
```python
if user.get_operand(i) == old_value:  # Works!
```

This was a pleasant surprise - the bindings properly implement value equality.

### Builder Reuse Across Blocks

The builder can be repositioned to different blocks without creating a new one:
```python
with bb1.create_builder() as builder:
    builder.add(a, b, "sum")
    builder.position_at_end(bb2)  # Move to different block
    builder.sub(a, b, "diff")     # Works in bb2
```

### Function Parameters

**C++:**
```cpp
Value* param = func->getArg(0);
```

**Python:**
```python
param = func.get_param(0)
```

### Instruction Name Access

Both getting and setting names work:
```python
name = inst.name        # Get name
inst.name = "new_name"  # Set name (but may not work for all values)
```

### Global Variable Value Type

To get the type of what a global contains (not the pointer type):
```python
gv.type           # Returns ptr (pointer to the global)
gv.global_value_type  # Returns the actual content type, e.g., [14 x i8]
```

### Select Instruction

The select instruction (ternary) is available:
```python
result = builder.select(condition, true_val, false_val, "name")
```

---

## API Review and Recommendations

### What Works Well

1. **Builder API**: The builder API is well-designed and intuitive. Method names are clean (`add`, `sub`, `mul` instead of `CreateAdd`, etc.)

2. **Context Managers**: Using context managers for modules and builders prevents memory leaks and is Pythonic.

3. **Property Access**: Using properties for simple getters (`inst.opcode`, `bb.terminator`) feels natural in Python.

4. **Type Factory**: `ctx.types.i32`, `ctx.types.function(...)` is a nice abstraction.

5. **Iteration**: Iterating over functions, blocks, and instructions with `for` loops works seamlessly.

### What Needs Improvement

#### Critical Issues

1. **UTF-8 String Encoding Bug**: `const_string` and `const_data_array` encode strings as UTF-8, making it impossible to work with raw byte arrays. This is a **blocking issue** for any binary manipulation use case. 
   
   **Recommendation**: Add `const_bytes(ctx, bytes_object)` or accept `bytes` type in addition to `str`.

2. **Missing `replaceAllUsesWith`**: This is a fundamental LLVM operation used constantly in transforms. Its absence forces users to implement it manually (error-prone).
   
   **Recommendation**: Add `value.replace_all_uses_with(new_value)`.

3. **Missing `splitBasicBlock`**: Cannot split blocks, which is essential for many transformations.
   
   **Recommendation**: Add `bb.split_before(inst)` returning the new block.

4. **Two-Step Instruction Deletion**: Having to call `remove_from_parent()` then `delete_instruction()` is error-prone and non-obvious.
   
   **Recommendation**: Add `inst.erase_from_parent()` that does both, matching C++ API.

#### API Inconsistencies

1. **`ptr()` vs `i32`**: Why is `ctx.types.ptr()` a method but `ctx.types.i32` a property? This inconsistency causes confusion.
   
   **Recommendation**: Make all types properties, or all methods. Consistency matters more than either choice.

2. **`set_constant()` vs `linkage =`**: Global variable `is_constant` uses a setter method, but `linkage` uses property assignment. Pick one pattern.
   
   **Recommendation**: Prefer property setters: `gv.is_constant = False`.

3. **`.block` vs `.parent`**: Instructions use `.block` to get parent, but the C++ API and intuition suggest `.parent`.
   
   **Recommendation**: Add `.parent` as an alias for `.block`.

4. **`.is_terminator_inst` suffix**: The `_inst` suffix is inconsistent with other boolean properties.
   
   **Recommendation**: Use `.is_terminator` instead.

#### Missing Conveniences

1. **No `.operands` iterator**: Having to use index-based access is verbose:
   ```python
   # Current (verbose)
   for i in range(inst.num_operands):
       op = inst.get_operand(i)
   
   # Desired
   for op in inst.operands:
   ```
   
   **Recommendation**: Add `operands` property returning an iterator.

2. **No instruction movement**: Can't move instructions between blocks or reorder them.
   
   **Recommendation**: Add `inst.move_before(other)`, `inst.move_after(other)`.

3. **No instruction cloning**: Can't clone an instruction.
   
   **Recommendation**: Add `inst.clone()`.

### Documentation Gaps

1. **No comprehensive API reference**: Had to discover APIs through `dir()` and trial-and-error.

2. **Exception types not documented**: Had to guess that `LLVMError` exists, not `LLVMException`.

3. **Context manager behavior not obvious**: The `ModuleManager` pattern needs explicit documentation.

### Overall Assessment

**Rating: 7/10 for basic use, 5/10 for advanced transforms**

The bindings are **good for code generation** (creating new IR from scratch) where the Builder API shines. They're **challenging for transforms** (modifying existing IR) due to missing fundamental operations like RAUW, block splitting, and instruction movement.

The API is **Pythonic in places** (properties, context managers, iteration) but **inconsistent in others** (method vs property for similar operations). The UTF-8 string bug is a **critical issue** that blocks an entire category of use cases.

For someone porting C++ LLVM code, expect to:
- Spend significant time discovering API names through trial-and-error
- Implement workarounds for missing functionality
- Hit crashes that require debugging LLVM assertions
- Accept that some transformations simply aren't possible

---

## Version Information

This guide was written based on porting experience with:
- llvm-nanobind Python bindings
- LLVM 21 (development version)
- Python 3.14

APIs may differ in other versions.
