# Known LLVM-C API Bugs and Limitations

This document tracks bugs and limitations discovered in the upstream LLVM-C API during development of llvm-nanobind.

## Bugs in LLVM-C API

### 1. Poison Value - Incorrect ValueKind (Confirmed in C and Python)

**Status**: Confirmed in both C and Python implementations  
**Severity**: High  
**LLVM Version**: 21.1.8

**Description**: When cloning poison values using the echo pattern, `LLVMGetValueKind()` returns an incorrect value kind for poison constants, causing the `check_value_kind()` assertion to fail.

**Reproducer**:
```llvm
define i32 @test_poison() {
  ret i32 poison
}
```

**Error**:
```
LLVM ERROR: LLVMGetValueKind returned incorrect type
```

**Location in Code**:
- C: `llvm-c/llvm-c-test/echo.cpp` line 368-371
- Python: `llvm_c_test/echo.py` line 226-228

**Impact**: Cannot use poison values in echo tests. This affects both the C binary and Python implementation equally.

**Workaround**: Skip testing poison values in echo command tests.

---

### 2. BitCast Instruction - Assertion Failure (Confirmed in C only)

**Status**: Confirmed in C implementation  
**Severity**: High  
**LLVM Version**: 21.1.8

**Description**: When cloning certain bitcast instructions, the C implementation crashes with an assertion failure in LLVM's casting infrastructure.

**Reproducer**:
```llvm
define ptr @test_bitcast(ptr %p) {
  %1 = bitcast ptr %p to ptr
  ret ptr %1
}
```

**Error**:
```
Assertion failed: (isa<To>(Val) && "cast<Ty>() argument of incompatible type!"), 
function cast, file Casting.h, line 578.
```

**Location in Code**:
- C: `llvm-c/llvm-c-test/echo.cpp` line 819-822

**Impact**: Cannot test bitcast instructions in echo command, even though bitcast cloning code exists.

**Note**: With opaque pointers, ptr-to-ptr bitcast is mostly a no-op, so this is less critical. The existing echo.ll test doesn't include any bitcast instructions.

**Workaround**: Skip bitcast instruction tests. The constant expression bitcast path can still be tested via GEP expressions.

---

### 3. ConstantFP - Not Supported (By Design)

**Status**: Intentional limitation  
**Severity**: High (but documented)  
**LLVM Version**: All versions

**Description**: The LLVM-C API does not provide a way to extract the actual floating-point value from a ConstantFP to recreate it in another context. Both C and Python implementations explicitly throw an error.

**Reproducer**:
```llvm
@float_const = global <2 x float> <float 1.0, float 2.0>
```

**Error**:
```
LLVM ERROR: ConstantFP is not supported
```

**Location in Code**:
- C: `llvm-c/llvm-c-test/echo.cpp` line 381-384
- Python: `llvm_c_test/echo.py` line 237-239

**Impact**: Cannot test floating-point literal constants. Integer representations work (via bitcasting to int).

**Workaround**: Avoid float literals in echo tests. Use integer vectors or other constant types instead.

---

### 4. BFloat Type - Incorrectly Cloned as Half (FIXED)

**Status**: ✅ FIXED in both C and Python implementations  
**Severity**: Medium  
**LLVM Version**: 21.1.8

**Description**: In the reference C implementation of echo, the `LLVMBFloatTypeKind` case incorrectly returned `LLVMHalfTypeInContext()` instead of `LLVMBFloatTypeInContext()`.

**Location in Code**:
- C (FIXED): `llvm-c/llvm-c-test/echo.cpp` line 78-79:
  ```cpp
  case LLVMBFloatTypeKind:
    return LLVMBFloatTypeInContext(Ctx);  // FIXED
  ```
- Python (correct): `llvm_c_test/echo.py` line 40-41:
  ```python
  elif kind == llvm.TypeKind.BFloat:
      return self.ctx.bfloat_type()  # CORRECT
  ```

**Reproducer**:
```llvm
define bfloat @test_bfloat(bfloat %x) {
  %1 = fadd bfloat %x, %x
  ret bfloat %1
}
```

**Result**:
- ✅ Both C and Python now correctly output: `define bfloat @test_bfloat(bfloat %x)`

**Fix Applied**: Changed line 79 in echo.cpp from `LLVMHalfTypeInContext(Ctx)` to `LLVMBFloatTypeInContext(Ctx)`.

---

## Limitations (Not Bugs)

### x86_amx Type Restrictions

**Description**: The `x86_amx` type can only be used in specific LLVM intrinsics, not in regular function signatures or alloca instructions.

**Impact**: Cannot create comprehensive x86_amx type tests.

**Workaround**: Skip x86_amx type testing. The type kind is recognized but cannot be used in test IR.

---

## Testing Implications

Due to these bugs and limitations, the following coverage paths remain untested in llvm_c_test/echo.py:

- **Poison value handling** (lines 226-228) - LLVM-C bug #1
- **Float literal handling** (lines 238-239) - LLVM-C limitation #3  
- **BitCast instruction** (lines 689-690) - LLVM-C bug #2
- **x86_amx type** (line 104) - LLVM limitation

These are **not** bugs in llvm-nanobind, but rather limitations in the upstream LLVM-C API that affect both the reference C implementation and our Python bindings equally.

---

## Reporting Status

- [ ] Report poison value issue to LLVM project
- [ ] Report bitcast crash to LLVM project
- [x] Document ConstantFP limitation (already well-known)

---

**Last Updated**: 2024-12-19
