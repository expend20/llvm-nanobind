# Plan: Fix Python Implementation Test Failures

## Current Status

When running lit tests with `--use-python`, 10 out of 23 tests fail:

```
Failed Tests (10):
  llvm-c-test :: atomics.ll
  llvm-c-test :: debug_info_new_format.ll
  llvm-c-test :: echo.ll
  llvm-c-test :: empty.ll
  llvm-c-test :: float_ops.ll
  llvm-c-test :: freeze.ll
  llvm-c-test :: functions.ll
  llvm-c-test :: invalid-bitcode.test
  llvm-c-test :: invoke.ll
  llvm-c-test :: memops.ll
```

## Analysis

### Root Cause Categories

1. **Echo command failures** (atomics.ll, echo.ll, float_ops.ll, freeze.ll, invoke.ll, memops.ll)
   - Error: `RuntimeError: Expected an instruction` in `clone_value`
   - Likely cause: Operand handling for certain instruction types (fence, atomics, etc.)
   - The `clone_value` function expects instructions but receives other value types

2. **Diagnostic handler crash** (empty.ll)
   - Error: `Trace/BPT trap: 5` when running `--test-diagnostic-handler`
   - Likely cause: Issue with the diagnostic handler implementation

3. **Module list functions** (functions.ll)
   - Needs investigation

4. **Invalid bitcode handling** (invalid-bitcode.test)
   - Needs investigation - may be error message format difference

5. **Debug info** (debug_info_new_format.ll)
   - Complex test that exercises DIBuilder extensively

## Approach

### Phase 1: Investigate and categorize failures

For each failing test:
1. Run the test manually to capture exact error
2. Compare Python output with C output
3. Identify the specific issue

### Phase 2: Fix echo command issues

The echo command has the most failures. Key areas to investigate:

1. **clone_value function** (`echo.py:405`)
   - Check if it handles all value types correctly
   - May need to handle constants, arguments, and other non-instruction values

2. **Instruction opcode handlers**
   - Verify all opcodes are handled
   - Check for missing opcodes between Python and C enum values

3. **Operand handling**
   - Some instructions (like fence) have no operands
   - Some operands are not instructions (constants, arguments)

### Phase 3: Fix other command issues

1. **Diagnostic handler** - Check thread-local storage and callback handling
2. **Module operations** - Compare output format
3. **Invalid bitcode** - Check error message format

## Testing Strategy

```bash
# Run specific test with verbose output
uv run python run_llvm_c_tests.py --use-python -v atomics.ll

# Test individual command manually
cat llvm-c/llvm-c-test/inputs/atomics.ll | $(brew --prefix llvm)/bin/llvm-as | \
  uv run python llvm-c-test-wrapper.py --echo

# Compare with C version
cat llvm-c/llvm-c-test/inputs/atomics.ll | $(brew --prefix llvm)/bin/llvm-as | \
  ./build/llvm-c-test --echo
```

## Success Criteria

All 23 lit tests pass with `--use-python`:
```bash
uv run python run_llvm_c_tests.py --use-python
# Expected: 23 tests passed
```
