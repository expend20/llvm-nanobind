# Review Task: End-User Readiness Inventory

This document captures the comprehensive review of the llvm-nanobind codebase to identify what's needed before the project is ready for end-users.

## Executive Summary

The llvm-nanobind project is more mature than its README suggests. Core bindings are comprehensive, the llvm-c-test Python port is ~90% complete, and memory safety is well-handled. However, test coverage needs improvement (currently 36% for llvm_c_test), documentation needs updates, and realistic examples are missing.

---

## Current State Assessment

### Strengths

| Area | Status | Notes |
|------|--------|-------|
| Core bindings | ~7300 lines | Comprehensive LLVM-C coverage |
| Golden master tests | 15 pairs | C++/Python test pairs |
| llvm-c-test port | 19/21 commands | 90% complete |
| Memory safety | Excellent | Validity tokens, lifetime management |
| Type checking | Working | Auto-generated stubs, ty/pyright pass |
| Lit tests | 23 passing | Full parity with C implementation |

### Areas Needing Work

| Area | Current State | Target |
|------|---------------|--------|
| llvm_c_test coverage | 36% | 90%+ |
| echo.py coverage | 23% | 80%+ |
| Documentation | Outdated | Current |
| Examples | Minimal playground.py | Realistic transform examples |
| README | "Early design phase" | Honest description |

---

## Priority 1: llvm-c-test Coverage (Target: 90%+)

This is the foundation - the llvm-c-test Python port is our primary evidence that bindings are comprehensive. Low coverage = low user confidence.

### Switch/IndirectBr Status

**Finding**: The upstream C implementation (`llvm-c/llvm-c-test/echo.cpp`) also has empty cases for Switch and IndirectBr:
```cpp
case LLVMSwitch:
case LLVMIndirectBr:
    break;
```

**Action**: We already have parity. Going beyond this would require upstream work. Document this as a known limitation.

### New Lit Tests Needed

| Test File | Instructions/Features | Priority |
|-----------|----------------------|----------|
| `switch.ll` | Switch instruction with multiple cases | High (if we implement) |
| `indirectbr.ll` | Indirect branch with block addresses | High (if we implement) |
| `vaarg.ll` | Variadic functions and va_arg instruction | Medium |
| `addrspace.ll` | Address space casts | Medium |
| `bfloat.ll` | bfloat16 type operations | Low |

### Incomplete Commands to Complete

| Command | File | Status |
|---------|------|--------|
| `--replace-md-operand` | metadata.py | Stub - needs implementation |
| `--is-a-value-as-metadata` | metadata.py | Basic stub - needs verification |

### Coverage Gap Analysis

**High-coverage files (>70%)**:
- disassemble.py: 89%
- metadata.py: 80%
- attributes.py: 74%

**Low-coverage files (<50%)**:
- echo.py: 23% ← Focus area
- helpers.py: 11%
- module_ops.py: 38%
- calc.py: 45%

**Strategy**: Create more lit tests that exercise untested code paths in echo.py. This is the largest module (1409 lines) and has the most potential for improvement.

---

## Priority 2: Realistic Examples from Selene

The `tests/` directory focuses on **building IR from scratch**. We need examples that demonstrate **transforming existing IR** - a more realistic use case for obfuscators, analyzers, and compilers.

### Source Material

Reference code is in `reference/selene/` with plan at `reference/selene/plan.md`.

### Example Extraction Order (Simplest First)

1. **Module Iteration** - Iterate functions, globals, basic blocks, instructions
   - Source: `llvm.hpp` (iterator helpers)
   - APIs: `LLVMGetFirstFunction`, `LLVMGetNextFunction`, etc.
   - Complexity: Simple

2. **Cleanup Transform** - Remove unused globals and functions
   - Source: `transform/cleanup.cpp`
   - APIs: `LLVMGetFirstUse`, `LLVMDeleteFunction`, `LLVMDeleteGlobal`
   - Complexity: Simple

3. **Function Instrumentation** - Insert profiler calls
   - Source: `transform/profiler.cpp`
   - APIs: `LLVMAddFunction`, `LLVMBuildCall2`, `LLVMBuildGlobalString`
   - Complexity: Medium

4. **MBA Obfuscation** - Replace arithmetic with obfuscated expressions
   - Source: `transform/mixed_bool_arith.cpp`
   - APIs: `LLVMBuildNot`, `LLVMBuildAnd`, `LLVMReplaceAllUsesWith`
   - Complexity: Medium

5. **String Encryption** - Encrypt string constants with decryption stubs
   - Source: `transform/string_conversion.cpp`
   - APIs: `LLVMBuildPhi`, `LLVMAddIncoming`, `LLVMBuildAtomicRMW`
   - Complexity: Complex

### Golden Master Approach

Each example will have:
```
examples/selene/<example>/
├── <example>.cpp      # Standalone C++ using LLVM-C API
├── input.ll           # Test input (can generate with clang if needed)
├── expected.txt       # Expected output (IR + transformation log)
└── <example>.py       # Python port
```

Output should include:
1. Transformed IR
2. What was encountered in what order (for debugging/verification)

---

## Priority 3: Documentation Fixes

### Trivial Fixes

| File | Issue | Fix |
|------|-------|-----|
| `devdocs/DEBUGGING.md:348` | References non-existent `devdocs/fixing-tests/plan.md` | Change to `devdocs/archive/fixing-tests.md` |
| `devdocs/README.md` | Missing `fixing-tests.md` in archive listing | Add to archive table |

### README.md Overhaul

Current:
> "⚠️ This project is still in a very early design phase and not remotely usable."

Proposed:
> "Python bindings for the LLVM-C API using nanobind. Provides a Pythonic interface to LLVM's compiler infrastructure for building compilers, analyzers, and code transformation tools."
>
> **Status**: Under active development. Core APIs are bound and tested against the llvm-c-test suite, but the API is not yet stable.

---

## Not In Scope (See devdocs/future.md)

| Item | Reason |
|------|--------|
| JIT examples | JIT bindings not implemented |
| CI/CD pipeline | Deferred |
| API documentation generation | .pyi stubs are sufficient for now |
| Comprehensive contributor docs | Focus on end-user readiness first |

---

## Implementation Order

1. ✅ Create this plan.md
2. Create progress.md
3. Create devdocs/future.md (out-of-scope items)
4. Fix trivial devdocs issues
5. Create new lit tests for coverage gaps
6. Complete metadata.py stubs
7. Extract first selene example (module iteration)
8. Update README.md

---

## Success Criteria

- [ ] llvm_c_test coverage ≥90%
- [ ] echo.py coverage ≥80%
- [ ] metadata.py commands fully implemented
- [ ] At least 2 selene examples ported (simple + medium)
- [ ] README accurately describes project state
- [ ] All devdocs references are valid
- [ ] `uv run run_llvm_c_tests.py` passes
- [ ] `uvx ty check` passes
