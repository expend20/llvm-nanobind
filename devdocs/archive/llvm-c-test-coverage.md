# llvm-c-test Coverage Improvements

## Goal

Add missing cast operation handlers to `echo.cpp` so the C binary passes all lit tests, enabling the improvements to be submitted upstream to LLVM.

## Summary

The `casts.ll` test file was added to test cast operations, but only the Python implementation had handlers. The C binary failed with "30 is not a supported opcode" (LLVMTrunc). Added 10 missing cast handlers to achieve 25/25 test pass rate.

## Changes Made

Added handlers in `echo.cpp` (after LLVMZExt, around line 1017):

| Opcode | LLVM-C Builder Function |
|--------|-------------------------|
| `LLVMTrunc` | `LLVMBuildTrunc` |
| `LLVMSExt` | `LLVMBuildSExt` |
| `LLVMFPToUI` | `LLVMBuildFPToUI` |
| `LLVMFPToSI` | `LLVMBuildFPToSI` |
| `LLVMUIToFP` | `LLVMBuildUIToFP` |
| `LLVMSIToFP` | `LLVMBuildSIToFP` |
| `LLVMFPTrunc` | `LLVMBuildFPTrunc` |
| `LLVMFPExt` | `LLVMBuildFPExt` |
| `LLVMPtrToInt` | `LLVMBuildPtrToInt` |
| `LLVMIntToPtr` | `LLVMBuildIntToPtr` |

## Implementation Pattern

Each cast handler follows the same simple pattern:

```cpp
case LLVMTrunc: {
  LLVMValueRef Val = CloneValue(LLVMGetOperand(Src, 0));
  LLVMTypeRef DestTy = CloneType(LLVMTypeOf(Src));
  Dst = LLVMBuildTrunc(Builder, Val, DestTy, Name);
  break;
}
```

Note: `LLVMZExt` is special - it also preserves the `nneg` flag. Other casts don't have this.

## Test Results

| Implementation | Before | After |
|----------------|--------|-------|
| C binary | 24/25 | 25/25 |
| Python | 25/25 | 25/25 |

## Files Modified

- `llvm-c/llvm-c-test/echo.cpp` - Added 10 cast handlers
- `llvm-c/llvm-c-test/inputs/casts.ll` - New test file (was untracked)
- `llvm-c/llvm-c-test/inputs/targets.test` - New test file (was untracked)
