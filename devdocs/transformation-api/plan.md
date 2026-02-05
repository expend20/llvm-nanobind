# Transformation API Improvements

## Overview

This task tracks improvements to the Python bindings to better support IR transformation use cases (as opposed to just code generation). These issues were discovered while porting LLVM obfuscator passes to Python.

See `devdocs/porting-guide.md` for the full context and detailed examples.

## Priority 1: Critical Blockers

These issues completely block certain use cases.

### 1.1 Raw Bytes Support for Constants

**Problem**: `const_string` and `const_data_array` encode strings as UTF-8, causing byte sequences with values > 127 to expand. This makes binary manipulation impossible.

```python
# This creates [22 x i8] instead of [14 x i8]!
encrypted = bytes([0xFF, 0x80, 0x42, ...]).decode('latin-1')
const = llvm.const_string(ctx, encrypted, dont_null_terminate=True)
```

**Solution**: Accept `bytes` type in addition to `str`, passing raw bytes to LLVM without encoding.

```python
# Proposed API
const = llvm.const_bytes(ctx, b'\xff\x80\x42...')
# or
const = llvm.const_data_array(i8, b'\xff\x80\x42...')  # Accept bytes
```

**Files to modify**: Bindings for `LLVMConstStringInContext2`, `LLVMConstArray`

**Blocked use case**: String encryption, binary patching, any byte-level manipulation

---

### 1.2 Add `Value.replace_all_uses_with()`

**Problem**: No way to replace all uses of a value. Users must implement manually:

```python
# Current workaround (error-prone)
def replace_all_uses_with(old_value, new_value):
    for use in list(old_value.uses):
        user = use.user
        for i in range(user.num_operands):
            if user.get_operand(i) == old_value:
                user.set_operand(i, new_value)
```

**Solution**: Bind `LLVMReplaceAllUsesWith`.

```python
# Proposed API
old_inst.replace_all_uses_with(new_value)
```

**C API**: `LLVMReplaceAllUsesWith(LLVMValueRef OldVal, LLVMValueRef NewVal)`

**Blocked use case**: Any instruction replacement/transformation

---

### 1.3 Add `BasicBlock.split_before()`

**Problem**: Cannot split a basic block at an instruction point.

**Solution**: Bind `LLVMSplitBasicBlock` or implement via C++ API.

```python
# Proposed API
new_bb = bb.split_before(inst)  # Returns new block containing inst onwards
```

**C API**: Not directly available - may need custom C wrapper or use `LLVMInsertBasicBlock` + instruction movement.

**Blocked use case**: Basic block splitting passes, some control flow transforms

---

### 1.4 Single-Step Instruction Deletion

**Problem**: Deleting an instruction requires two calls, and doing it wrong crashes:

```python
# Current (crashes if order wrong)
inst.remove_from_parent()
inst.delete_instruction()

# If you forget remove_from_parent():
# Assertion failed: (!getParent() && "Instruction still linked in the program!")
```

**Solution**: Add `erase_from_parent()` that does both atomically.

```python
# Proposed API
inst.erase_from_parent()  # Unlinks and deletes in one call
```

**C API**: `LLVMInstructionEraseFromParent` (already exists!)

**Files to modify**: Check if this is bound; if not, add binding

---

## Priority 2: API Consistency

These issues cause confusion and bugs but have workarounds.

### 2.1 Make `ptr` a Property Like Other Types

**Problem**: `ctx.types.ptr()` is a method, but `ctx.types.i32` is a property.

```python
ptr_ty = ctx.types.ptr   # WRONG - returns bound method!
ptr_ty = ctx.types.ptr() # Correct

i32_ty = ctx.types.i32   # Correct - returns type directly
```

**Solution**: Make `ptr` a property that returns the opaque pointer type.

```python
# Proposed API
ptr_ty = ctx.types.ptr  # Property, like i32
```

**Note**: If `ptr(addrspace)` is needed for address spaces, keep method but add property for default.

---

### 2.2 Consistent Setter Patterns

**Problem**: Mixed patterns for setting properties on globals:

```python
gv.set_constant(False)      # Method
gv.linkage = llvm.Linkage.X # Property setter
```

**Solution**: Use property setters consistently.

```python
# Proposed API
gv.is_constant = False
gv.linkage = llvm.Linkage.X
```

---

### 2.3 Add `.parent` Alias for Instructions

**Problem**: Instructions use `.block` to get parent, but `.parent` is more intuitive and matches C++.

```python
bb = inst.block   # Current
bb = inst.parent  # Expected (doesn't exist)
```

**Solution**: Add `.parent` as an alias for `.block`.

---

### 2.4 Rename `.is_terminator_inst` to `.is_terminator`

**Problem**: The `_inst` suffix is inconsistent with other boolean properties.

```python
inst.is_terminator_inst  # Current
inst.is_terminator       # Expected
```

**Solution**: Add `.is_terminator` (keep old name for compatibility or deprecate).

---

## Priority 3: Missing Conveniences

These would make the API more Pythonic and reduce boilerplate.

### 3.1 Add `.operands` Iterator

**Problem**: Must use index-based access to iterate operands.

```python
# Current (verbose)
for i in range(inst.num_operands):
    op = inst.get_operand(i)

# Desired
for op in inst.operands:
```

**Solution**: Add `operands` property returning an iterator.

```python
# Proposed API
for op in inst.operands:
    process(op)

# Also useful:
ops = list(inst.operands)
```

---

### 3.2 Add Instruction Movement

**Problem**: Cannot move instructions between blocks or reorder within a block.

**Solution**: Add movement methods.

```python
# Proposed API
inst.move_before(other_inst)
inst.move_after(other_inst)
```

**C API**: `LLVMInstructionRemoveFromParent` + `LLVMInsertIntoBuilder` (partial support)

---

### 3.3 Add Instruction Cloning

**Problem**: Cannot clone an instruction.

**Solution**: Bind instruction cloning.

```python
# Proposed API
new_inst = inst.clone()
```

**C API**: `LLVMInstructionClone`

---

### 3.4 Add `.successors` Count

**Problem**: Must convert to list to count successors.

```python
# Current
num = len(list(term.successors))

# Desired
num = term.num_successors
```

**Note**: `num_successors` may already exist - verify.

---

## Priority 4: Documentation

### 4.1 Document Exception Types

Document available exceptions:
- `llvm.LLVMError` (base class)
- `llvm.LLVMParseError`
- `llvm.LLVMAssertionError`
- `llvm.LLVMMemoryError`

### 4.2 Document Context Manager Patterns

Explicitly document that `ctx.parse_ir()` and `ctx.create_module()` return context managers, not direct objects.

### 4.3 Add API Reference

Generate or write comprehensive API reference documentation.

---

## Implementation Order

Suggested implementation order based on impact and effort:

### Phase 1: Quick Wins (Low effort, High impact)
1. Bind `LLVMInstructionEraseFromParent` (1.4)
2. Bind `LLVMReplaceAllUsesWith` (1.2)
3. Add `.operands` iterator (3.1)
4. Add `.parent` alias (2.3)

### Phase 2: API Consistency
1. Make `ptr` a property (2.1)
2. Consistent setters (2.2)
3. Rename `is_terminator_inst` (2.4)

### Phase 3: Major Features
1. Raw bytes support (1.1) - requires design decision
2. Basic block splitting (1.3) - may need C wrapper
3. Instruction movement (3.2)
4. Instruction cloning (3.3)

### Phase 4: Documentation
1. Exception docs (4.1)
2. Context manager docs (4.2)
3. API reference (4.3)

---

## Testing Strategy

Each improvement should include:

1. **Unit test**: Direct test of the new API
2. **Integration test**: Use in a realistic transformation
3. **Porting test**: Verify it enables previously-blocked obfuscation pass functionality

Example test for `replace_all_uses_with`:
```python
def test_replace_all_uses_with():
    with llvm.create_context() as ctx:
        with ctx.create_module("test") as mod:
            # Create: %y = add %x, 1; %z = mul %y, 2
            # Replace %y with constant 42
            # Verify %z now uses 42
```

---

## References

- `devdocs/porting-guide.md` - Full porting experience documentation
- `tools/obfuscation/` - Ported passes showing workarounds
- LLVM C API: https://llvm.org/doxygen/group__LLVMC.html
