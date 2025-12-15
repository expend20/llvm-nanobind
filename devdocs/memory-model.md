# LLVM Python Bindings: Lifetime Management Strategy

## The Problem

LLVM has a strict parent-owns-children model:

```
Module
  └── Function
        └── BasicBlock
              └── Instruction
```

When a parent is destroyed, all children become invalid. In C++, accessing a child after its parent is destroyed causes a segfault. We need to detect this in Python and raise an exception instead.

Additionally, objects can be:
- **Detached**: Removed from parent but still alive (caller owns it)
- **Erased**: Removed from parent and deleted

Python's garbage collector provides no ordering guarantees, so we can't rely on destructor order.

---

## Solution: Validity Tokens

Every wrapper holds a `shared_ptr<ValidityToken>` pointing to its parent's token. When a parent is destroyed, it invalidates its token, and all children detect this on their next access.

```cpp
struct ValidityToken {
    std::atomic<bool> valid{true};
    void invalidate() { valid = false; }
    bool is_valid() const { return valid; }
};
```

### Token Hierarchy

```
Context Token ──────────────────────────────────┐
    │                                           │
Module Token ───────────────────────────┐       │
    │                                   │       │
    ├── Function (shares module token) ─┼───────┤
    │       │                           │       │
    │       └── BasicBlock ─────────────┼───────┤
    │               │                   │       │
    │               └── Instruction ────┘       │
    │                                           │
Builder Token ──────────────────────────────────┘
```

Each object checks **all ancestor tokens** before any operation:

```cpp
void LLVMInstruction::check_valid() const {
    if (!m_ref)
        throw LLVMError("Instruction has been erased");
    if (m_detached)
        return;  // Detached instructions have no parent to check
    if (!m_block_token || !m_block_token->is_valid())
        throw LLVMError("Instruction's basic block has been erased");
    if (!m_func_token || !m_func_token->is_valid())
        throw LLVMError("Instruction's function has been erased");
    if (!m_module_token || !m_module_token->is_valid())
        throw LLVMError("Instruction's module has been disposed");
    if (!m_context_token || !m_context_token->is_valid())
        throw LLVMError("Instruction's context has been disposed");
}
```

---

## Object States

### Module States

| State | Description | Owned By | On Destruction |
|-------|-------------|----------|----------------|
| Active | Inside `with` block or after `create_module()` | Manager/Handle | Disposed |
| Disposed | After `__exit__`, `.dispose()`, or `del` | None | Nothing |

### Function/BasicBlock/Instruction States

| State | Description | Owned By | On Destruction |
|-------|-------------|----------|----------------|
| Attached | Normal state, has parent | Parent | Nothing (parent will clean up) |
| Detached | After `.detach()`, no parent | Python wrapper | `LLVMDeleteX()` called |
| Erased | After `.erase()` | None (`m_ref=nullptr`) | Nothing |

---

## Python API Patterns

### Context and Module Lifecycle

```python
import llvm

# Pattern 1: Context manager (recommended)
with llvm.create_context() as ctx:
    with ctx.create_module("example") as mod:
        func = mod.add_function("foo", func_type)
        # mod is valid here
    # mod is now disposed
    
    # This raises: "Module has been disposed"
    print(mod.name)
# ctx is now disposed

# Pattern 2: Explicit .dispose() method
with llvm.create_context() as ctx:
    mod_manager = ctx.create_module("example")
    mod_manager.dispose()

    # This raises: "Module has already been disposed"
    mod_manager.dispose()

# Pattern 3: Forget to dispose
with llvm.create_context() as ctx:
    mod_manager = ctx.create_module("example")
# This raises: "Module has never been entered"

# Pattern 4: Double dispose
with llvm.create_context() as ctx:
    mod_manager = ctx.create_module("example")
    with mod_manager as mod:
        print(mod.name)
    # This raises: module has already been disposed
    mod_manager.dispose()
```

### Holding Invalid References

```python
with llvm.create_context() as ctx:
    with ctx.create_module("example") as mod:
        func = mod.add_function("foo", func_type)
        entry = func.append_basic_block("entry")
        
        with ctx.create_builder() as builder:
            builder.position_at_end(entry)
            inst = builder.add(a, b, name="sum")
        
        # Save references
        saved_func = func
        saved_block = entry
        saved_inst = inst

# Everything is disposed now. Accessing raises exceptions:

saved_inst.name      # LLVMError: Instruction's context has been disposed
saved_block.name     # LLVMError: BasicBlock's context has been disposed  
saved_func.name      # LLVMError: Function's context has been disposed
```

### Parent Erasure Invalidates Children

```python
with llvm.create_context() as ctx:
    with ctx.create_module("example") as mod:
        func = mod.add_function("foo", func_type)
        bb1 = func.append_basic_block("bb1")
        bb2 = func.append_basic_block("bb2")
        
        with ctx.create_builder() as builder:
            builder.position_at_end(bb1)
            inst1 = builder.add(a, b, name="sum")
            inst2 = builder.mul(inst1, c, name="prod")
        
        # Erase the basic block - all its instructions become invalid
        bb1.erase()
        
        inst1.name  # LLVMError: Instruction's basic block has been erased
        inst2.name  # LLVMError: Instruction's basic block has been erased
        
        # Erase the function - all its blocks become invalid
        func.erase()
        
        bb2.name    # LLVMError: BasicBlock's function has been erased
```

### Detach vs Erase

```python
with llvm.create_context() as ctx:
    with ctx.create_module("example") as mod:
        func = mod.add_function("foo", func_type)
        entry = func.append_basic_block("entry")
        other = func.append_basic_block("other")
        
        with ctx.create_builder() as builder:
            builder.position_at_end(entry)
            add = builder.add(a, b, name="sum")
            mul = builder.mul(add, c, name="prod")
            builder.ret(mul)
            
            # --- ERASE: Remove and delete ---
            mul.erase()  # Instruction is gone forever
            mul.name     # LLVMError: Instruction has been erased
            
            # --- DETACH: Remove but keep alive ---
            add.detach()  # Instruction is now "floating"
            add.is_detached  # True
            add.parent       # None
            add.name         # Still works! "sum"
            
            # Detached instruction can be reinserted
            builder.position_at_end(other)
            add.insert_into(builder)  # Now attached to 'other' block
            add.is_detached  # False
            add.parent       # <BasicBlock 'other'>
            
            # --- DETACH without reinsertion ---
            another = builder.sub(x, y, name="diff")
            another.detach()
            # If we don't reinsert, destructor will clean up:
            del another  # Calls LLVMDeleteInstruction internally

# Detaching a basic block
with llvm.create_context() as ctx:
    with ctx.create_module("example") as mod:
        func = mod.add_function("foo", func_type)
        bb = func.append_basic_block("movable")
        
        # Detach block from function
        bb.detach()
        bb.parent  # None
        bb.name    # Still works: "movable"
        
        # Reinsert into same or different function
        bb.insert_into(func)  # Back in the function
        
        # Or insert before/after another block
        bb.detach()
        bb.insert_before(other_block)
```

### Module Cloning

```python
with llvm.create_context() as ctx:
    with ctx.create_module("original") as mod:
        func = mod.add_function("foo", func_type)
        # ... build module ...
        
        # Clone returns a module manager (same as ctx.create_module)
        clone_manager = mod.clone()
    
    # Original module is disposed, but clone is still valid
    
    # Use cloned module with context manager
    with clone_manager as cloned:
        cloned.name  # "original"
        # ... modify clone ...
    # clone is disposed here
    # NOTE: without the `with` and `clone_manager.dispose()` we raise an exception
```

### Builder Lifetime

Builders have independent lifetime from the blocks they operate on:

```python
with llvm.create_context() as ctx:
    with ctx.create_module("example") as mod:
        func = mod.add_function("foo", func_type)
        entry = func.append_basic_block("entry")
        
        # Builder can outlive or be shorter-lived than blocks
        with ctx.create_builder() as builder:
            builder.position_at_end(entry)
            inst = builder.add(a, b)
        # Builder disposed, but entry and inst still valid
        
        inst.name  # Works fine
        
        # Can create new builder later
        with ctx.create_builder() as builder2:
            builder2.position_before(inst)
            # Insert more instructions
```

---

## Implementation Details

### Wrapper Structure

```cpp
struct LLVMInstruction : NoMoveCopy {
    LLVMValueRef m_ref = nullptr;
    
    // Token hierarchy - check all on access
    std::shared_ptr<ValidityToken> m_context_token;
    std::shared_ptr<ValidityToken> m_module_token;
    std::shared_ptr<ValidityToken> m_func_token;
    std::shared_ptr<ValidityToken> m_block_token;
    
    bool m_detached = false;
    
    void check_valid() const {
        if (!m_ref)
            throw LLVMError("Instruction has been erased");
        if (m_detached)
            return;  // No parent to validate
        if (!m_block_token || !m_block_token->is_valid())
            throw LLVMError("Instruction's basic block has been erased");
        if (!m_func_token || !m_func_token->is_valid())
            throw LLVMError("Instruction's function has been erased");
        if (!m_module_token || !m_module_token->is_valid())
            throw LLVMError("Instruction's module has been disposed");
        if (!m_context_token || !m_context_token->is_valid())
            throw LLVMError("Instruction's context has been disposed");
    }
    
    void erase() {
        check_valid();
        LLVMInstructionEraseFromParent(m_ref);
        m_ref = nullptr;
        clear_tokens();
    }
    
    void detach() {
        check_valid();
        if (m_detached)
            throw LLVMError("Instruction is already detached");
        LLVMInstructionRemoveFromParent(m_ref);
        m_detached = true;
        clear_tokens();  // No longer has parent
    }
    
    void insert_into(LLVMBuilder* builder) {
        if (!m_ref)
            throw LLVMError("Instruction has been erased");
        if (!m_detached)
            throw LLVMError("Instruction is not detached");
        builder->check_valid();
        
        LLVMInsertIntoBuilder(builder->m_ref, m_ref);
        m_detached = false;
        adopt_tokens_from(builder);  // Get new parent's tokens
    }
    
    ~LLVMInstruction() {
        // Only clean up if WE own it (detached state)
        if (m_detached && m_ref) {
            LLVMDeleteInstruction(m_ref);
        }
    }
};
```

### BasicBlock with Child Tracking

When a BasicBlock is erased, it must invalidate all Instruction wrappers:

```cpp
struct LLVMBasicBlock : NoMoveCopy {
    LLVMBasicBlockRef m_ref = nullptr;
    std::shared_ptr<ValidityToken> m_token;  // This block's token
    
    // Parent tokens
    std::shared_ptr<ValidityToken> m_context_token;
    std::shared_ptr<ValidityToken> m_module_token;
    std::shared_ptr<ValidityToken> m_func_token;
    
    bool m_detached = false;
    
    void erase() {
        check_valid();
        m_token->invalidate();  // Invalidates all child instructions
        LLVMDeleteBasicBlock(m_ref);
        m_ref = nullptr;
    }
    
    void detach() {
        check_valid();
        LLVMRemoveBasicBlockFromParent(m_ref);
        m_detached = true;
        // Keep m_token valid - children are still valid
        // Clear parent tokens
        m_func_token = nullptr;
        m_module_token = nullptr;
    }
    
    ~LLVMBasicBlock() {
        if (m_detached && m_ref) {
            m_token->invalidate();
            LLVMDeleteBasicBlock(m_ref);
        }
    }
};
```

---

## Summary Table

| Operation | Effect on Self | Effect on Children |
|-----------|---------------|-------------------|
| `module.dispose()` / `del module` | Invalidated | All functions/globals invalidated |
| `func.erase()` | Deleted | All blocks invalidated |
| `func.detach()` | Removed from module, still valid | Children still valid |
| `block.erase()` | Deleted | All instructions invalidated |
| `block.detach()` | Removed from function, still valid | Children still valid |
| `inst.erase()` | Deleted | N/A |
| `inst.detach()` | Removed from block, still valid | N/A |
| Exit `with` block | Disposed | All descendants invalidated |
| Context destroyed | Invalidated | Everything invalidated |