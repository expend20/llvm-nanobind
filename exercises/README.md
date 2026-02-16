# LLVM Python Workshop Exercise Plan

This folder is a planning scaffold for a workshop site that teaches LLVM through the new Python API.

## Goals

- Teach high-value LLVM concepts first, not full API coverage.
- Make mistakes visible through deliberate pitfall exercises.
- Support interactive validation in a browser-like portal with fixed input IR.
- Keep all checks deterministic and auto-gradable.

## API Surface Review (Priority Order)

The current bindings are broad, but workshop priority should follow usage and learning value:

1. Core construction and inspection
   - `Context`, `Module`, `TypeFactory`, `Type`, `Value`
   - `Function`, `BasicBlock`, `Builder`
2. Correctness workflows
   - `parse_ir`, `parse_bitcode_from_bytes`, `verify`, `get_verification_error`
3. Common compiler tasks
   - arithmetic, memory ops, control flow, PHI nodes, attributes, metadata
4. Optimization and target work
   - `run_passes`, `PassBuilderOptions`, target triple/data layout/codegen
5. Advanced tooling
   - debug info (`DIBuilder`), disassembly, object/binary iterators

Safety semantics should be taught early:
- `LLVMMemoryError` for lifetime violations
- `LLVMAssertionError` for programming mistakes
- `LLVMParseError` for parse failures + diagnostics

## Workshop Curriculum Shape

Recommended progression:

1. Foundations (IR reading/writing, module/function/block basics)
2. Builder fundamentals (arithmetic, memory, casts, compare)
3. Control flow + SSA (branches, loops, PHI, predecessors)
4. Module-level features (globals, linkage, attributes, metadata)
5. Optimization + target pipeline
6. Debug info + binary inspection
7. Capstone transformations

See `exercises/CATALOG.md` for a full exercise list.

## Web Portal Validation Model

Each exercise should provide:

- fixed `input.ll` (or explicit "build from scratch")
- a short task prompt
- starter Python function (single entrypoint)
- deterministic checks that return structured pass/fail

Validation modes:

- IR text check: normalize and compare exact expected output
- property check: parse result and assert structural properties
- safety check: assert the correct exception type/message class

Recommended deterministic policy:

- verify generated module with `mod.verify()`
- avoid target-dependent codegen in early exercises
- normalize trivial IR formatting differences before compare
- avoid nondeterministic names in expected output checks

## Proposed Folder Layout

```text
exercises/
  README.md
  CATALOG.md
  portal-schema.md
  templates/
    exercise.toml
    exercise_output.toml
```

As implementation begins, add:

```text
  01-foundations/
  02-builder/
  03-control-flow-ssa/
  04-module-attrs-metadata/
  05-passes-target/
  06-debug-binary/
  07-capstones/
```

## Implementation Phases

1. Build exercise loader + schema validation.
2. Implement 10 core exercises (Foundations + Builder).
3. Implement 10 pitfall-heavy exercises (SSA + lifetime + validation).
4. Implement 8 advanced exercises (passes/target/debug/binary).
5. Add scoring, hints, and solution diff views in portal.
