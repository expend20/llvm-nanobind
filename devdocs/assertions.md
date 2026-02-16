# Assertions and Validation Guide

## Purpose

This document defines how to design assertions/validation checks in the Python
bindings so that:

1. The Python process never crashes.
2. User mistakes produce clear Python exceptions.
3. Invalid operations do not leave LLVM IR in a half-modified state.

This is a practical guide for implementing and testing safety checks in
`src/llvm-nanobind.cpp`.

## Non-Negotiable Rules

1. No hard crash for recoverable misuse.
2. Validate everything that can be validated before mutating LLVM state.
3. If validation fails, the IR must remain byte-for-byte unchanged.
4. Every non-trivial guard needs a regression test.

## Error Model

Use exceptions to classify failure type:

- `LLVMMemoryError`: lifetime/use-after-dispose/null wrapper state.
- `LLVMAssertionError`: API misuse or IR rule violation by caller.
- `LLVMError`: runtime operation failures from LLVM APIs (I/O/parsing/etc.).

Validation errors should generally be `LLVMAssertionError` with explicit,
actionable messages.

## Validation Layers

### 1) Wrapper Lifetime and Null State

Always check wrapper validity first:

- Wrapper pointer exists (`m_ref != nullptr`)
- Context token is valid (`m_context_token`)

Do this in `check_valid()` and call it at method entry.

### 2) Operand/Argument Shape

Validate:

- Object categories (instruction vs type vs block)
- Index bounds
- Required parent relationships
- Non-null required operands

Example: `get_operand(index)` must check bounds and throw, never let LLVM
perform unchecked access.

### 3) Cross-Context Ownership

Reject cross-context operations unless explicitly supported.

Example: instruction move APIs must reject moving between different context
tokens:

- `Cannot move instructions across different contexts`

### 4) IR Placement and Structural Invariants

Enforce LLVM structural rules up front:

- PHI nodes must stay in the PHI prefix.
- Non-PHI instructions cannot be inserted before PHIs.
- LandingPad must be the first non-PHI in its block.
- Terminator placement constraints:
  - Terminator can only be inserted at end.
  - Non-terminator cannot be inserted after existing terminator.

### 5) Mutation Safety (No Half-Modified State)

The rule is:

`validate -> compute insertion state -> validate insertion -> mutate`

Do not unlink/remove/splice until all checks pass.

If you must derive insertion state that depends on current placement, derive it
without mutating first and handle adjacency/self-move cases explicitly.

## Transaction Pattern for Safe Mutations

For operations that may alter IR:

1. Validate input wrappers and types.
2. Validate context compatibility.
3. Compute destination and insertion point.
4. Validate insertion legality.
5. Only then mutate IR.
6. If any step fails before mutation, throw and return.

This prevents "reject + partially changed IR" bugs.

## Preserve Semantics

When the API supports `preserve: bool` for movement:

- `preserve=False`: normal insertion positioning.
- `preserve=True`: use debug-record-preserving insertion points where available
  (`LLVMPositionBuilderBeforeInstrAndDbgRecords` in C API).

Preserve mode should not weaken structural validation.

## High-Risk Footguns to Guard

### Instruction Movement

Risks:

- Cross-context moves
- Invalid placement (PHI/landingpad/terminator)
- Mutation before validation causing corrupt/partial state

Required guards:

- Category checks (`LLVMIsAInstruction`)
- Context token check
- Destination parent/module checks
- Placement validation
- Validate before unlink

### Iterator Semantics Over LLVM Iterators

Risks:

- Returning advanced/end iterator object to Python property access
- Undefined behavior or crashes in underlying C API

Required behavior:

- Python `__next__` returns current item.
- Advance between calls.
- End-state must raise `StopIteration` before property access.

### Binary/Object Parsing from Stdin

Risks:

- Corrupted input handling can trigger LLVM aborts
- Missing upfront format checks/clear errors

Required behavior:

- Read bytes paths only (`sys.stdin.buffer.read()` in Python tools)
- Wrap creation/parsing in exception handling
- Avoid exposing invalid iterator states

### Lifetime Escapes

Risks:

- Accessing values/types/builders after context/module disposal

Required behavior:

- `check_valid()` on every public operation
- Strong tests that escaped objects raise Python exceptions, not crashes

## Testing Strategy for Assertions

### 1) Positive + Negative Pairing

For each guarded API:

- Positive test: valid operation works.
- Negative test: invalid operation raises expected exception.

### 2) No-Mutation-on-Failure Assertions

For every negative move/mutate test:

1. Capture pre-state:
   - instruction order
   - parent block links
   - optionally full IR string (`mod.to_string()`)
2. Perform invalid op and assert exception.
3. Assert post-state equals pre-state.
4. `mod.verify()` must still pass.

### 3) Crash Repro Regressions

If a crash is found:

1. Add minimal pure-Python repro in `tests/regressions/`.
2. Add assertions for expected exception.
3. Keep test deterministic and fast.

### 4) Coverage Expectations

When adding non-trivial guards:

- Add or extend regression tests so both success and failure paths execute.
- Run:
  - `uv run run_tests.py`
  - `uv run run_tests.py --regressions`
  - `uv run run_llvm_c_tests.py --use-python` when affected.

## Current Safety Examples in This Repo

### Instruction Move Safety

Implemented validation includes:

- Context checks
- PHI/landingpad/terminator placement constraints
- Validation before unlinking/removal
- Optional `preserve` mode

See:

- `src/llvm-nanobind.cpp` (instruction movement and insertion validation)
- `tests/regressions/test_instruction_move_and_aliases.py`

### Iterator Safety

Binary/section/symbol/relocation iterators now follow safe Python iterator
protocol and avoid exposing advanced/end state as current items.

See:

- `src/llvm-nanobind.cpp` iterator `__next__` implementations
- `tests/regressions/test_binary_iterators.py`

### Raw Bytes Constant Safety

Byte-preserving constant APIs avoid UTF-8 expansion footguns for binary data
paths.

See:

- `tests/regressions/test_const_bytes.py`

## Assertion Message Quality

Good messages:

- Name the violated rule.
- Name the operand/position involved.
- Avoid generic "invalid argument".

Examples:

- `PHI nodes must be inserted in the PHI prefix of the basic block`
- `Cannot insert non-terminator instruction after block terminator`
- `Cannot move instructions across different contexts`

## Review Checklist (Before Merging)

- [ ] Did we validate all preconditions before mutation?
- [ ] Can the failure path leave partial IR changes?
- [ ] Do negative tests assert IR unchanged?
- [ ] Do tests cover both success and failure?
- [ ] Are exception types and messages specific?
- [ ] Did we run regression + golden master suites?
- [ ] For tool-path changes, did we run llvm-c lit tests in Python mode?

## Related Docs

- `devdocs/DEBUGGING.md`
- `devdocs/memory-model.md`
- `devdocs/lit-tests.md`
- `devdocs/api-reference.md`
