# Memory Model Issues

## Issue: Module Disposal After Context Destruction Causes Hard Crash

**Date Discovered:** December 17, 2025  
**Status:** Workaround in place, proper fix needed

### Problem Description

When an LLVM Module is garbage collected after its parent Context has been destroyed, the Python interpreter crashes with a segmentation fault:

```
Exception Type:        EXC_BAD_ACCESS (SIGSEGV)
Exception Codes:       KERN_INVALID_ADDRESS at 0x00008244894074e4 -> 0x00000244894074e4 (possible pointer authentication failure)

Thread 0 Crashed::  Dispatch queue: com.apple.main-thread
0   libLLVM.dylib    llvm::LLVMContext::removeModule(llvm::Module*) + 4
1   libLLVM.dylib    llvm::Module::~Module() + 48
2   libLLVM.dylib    LLVMDisposeModule + 16
3   __init__.abi3.so (our binding)
```

### Root Cause

LLVM modules maintain a back-pointer to their context. When `LLVMDisposeModule` is called, it tries to remove the module from the context's module list. If the context has already been destroyed, this causes a use-after-free crash.

### Reproduction Code

```python
import llvm

with llvm.create_context() as ctx:
    buf = llvm.create_memory_buffer_with_stdin()
    src = llvm.parse_bitcode_in_context(ctx, buf)  # Module created
    
    # ... do work with src ...
    
# Context destroyed here, but 'src' still exists
# Python GC eventually collects 'src', calling LLVMDisposeModule
# CRASH: context already freed
```

### Current Workaround

Explicitly delete the module before the context exits:

```python
with llvm.create_context() as ctx:
    buf = llvm.create_memory_buffer_with_stdin()
    src = llvm.parse_bitcode_in_context(ctx, buf)
    
    # ... do work with src ...
    
    output = str(m)  # Capture any needed output
    del src  # Explicitly delete before context exits

# Now context can safely exit
print(output)
```

### Proper Fix Options

#### Option 1: Track modules in context wrapper

The `LLVMContextWrapper` could maintain a list of all modules created in it. On context destruction, dispose all modules first:

```cpp
struct LLVMContextWrapper {
  std::vector<LLVMModuleWrapper*> m_modules;
  
  ~LLVMContextWrapper() {
    // Dispose all modules before context
    for (auto* mod : m_modules) {
      if (mod->m_ref) {
        LLVMDisposeModule(mod->m_ref);
        mod->m_ref = nullptr;
      }
    }
    // Now safe to dispose context
    LLVMContextDispose(m_ref);
  }
};
```

#### Option 2: Prevent double-free with validity token

We already have `ValidityToken` for tracking context lifetime. The module wrapper should check this before disposing:

```cpp
struct LLVMModuleWrapper {
  ~LLVMModuleWrapper() {
    if (m_ref && m_context_token && m_context_token->is_valid()) {
      LLVMDisposeModule(m_ref);  // Safe: context still alive
    }
    // If context is dead, just leak the module (already freed with context)
  }
};
```

**Problem with Option 2:** LLVM doesn't automatically free modules when a context is destroyed - we'd leak memory.

#### Option 3: Use shared ownership with poisoning

Make modules prevent context destruction while they exist:

```cpp
struct LLVMModuleWrapper {
  std::shared_ptr<LLVMContextWrapper> m_context;  // Shared ownership
  // Context can't be destroyed while any module holds a reference
};
```

**Problem:** This could cause surprising behavior where context `with` block doesn't actually free the context.

#### Option 4: Throw Python exception instead of crashing

At minimum, we should detect this condition and raise a Python exception:

```cpp
struct LLVMModuleWrapper {
  ~LLVMModuleWrapper() {
    if (m_ref) {
      if (!m_context_token || !m_context_token->is_valid()) {
        // Log warning - module outlived context
        // Can't throw in destructor, but can log
        fprintf(stderr, "Warning: Module disposed after context destroyed\n");
        m_ref = nullptr;  // Don't crash, just leak
        return;
      }
      LLVMDisposeModule(m_ref);
    }
  }
};
```

### Recommended Solution

**Option 4** is the minimum acceptable fix - never crash the interpreter.

**Option 1** is the ideal fix - context wrapper should own and manage all its modules.

### Affected APIs

Any API that creates a module associated with a context:
- `llvm.parse_bitcode_in_context(ctx, buf)`
- `ctx.create_module(name)` (already uses `with` pattern, safer)
- `llvm.get_bitcode_module_in_context(ctx, buf)`

### Test Case

```python
# test_module_lifetime.py
import llvm
import gc

def test_module_outlives_context_should_not_crash():
    """Module outliving context should not crash interpreter."""
    modules = []
    
    with llvm.create_context() as ctx:
        buf = llvm.create_memory_buffer_with_stdin()
        src = llvm.parse_bitcode_in_context(ctx, buf)
        modules.append(src)  # Keep reference outside context
    
    # Context destroyed, module still referenced
    gc.collect()  # Force GC - should not crash
    
    # Accessing module should raise LLVMUseAfterFreeError, not crash
    try:
        _ = modules[0].name
        assert False, "Should have raised exception"
    except llvm.LLVMUseAfterFreeError:
        pass  # Expected
```

### Related Files

- `src/llvm-nanobind.cpp`: `LLVMModuleWrapper`, `LLVMContextWrapper`
- `llvm_c_test/echo.py`: Contains workaround with explicit `del src`
