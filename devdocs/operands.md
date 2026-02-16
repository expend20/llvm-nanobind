# LLVM Instruction Operand Layout vs Printed IR

LLVM's internal operand storage order frequently diverges from the textual IR you see in `.ll` files. This is mostly due to a memory layout optimization: instructions with a variable number of operands (calls, branches, PHIs) use negative indexing from the end of the operand list (`Op<-1>`, `Op<-2>`, ...) so that fixed operands like the callee or branch targets always live at a known offset regardless of how many arguments or successors there are. The result is that raw `getOperand(i)` indices can be surprising — successors may be reversed, the callee may be last, and value/block pairs may be interleaved.

This document catalogs every instruction class, its internal operand layout, where it diverges from the printed IR, and the named accessor you should use instead. The golden rule: **if a named accessor exists, use it.** Treat `getOperand(i)` as an implementation detail.

**Legend:**
- ⚠️ = operand order differs from printed IR or is a common source of confusion
- ✅ = operand order matches printed IR straightforwardly

## Terminal Instructions

| Instruction | Printed IR | `getOperand(i)` layout | Gotcha | Preferred API |
|---|---|---|---|---|
| ⚠️ `BranchInst` (cond) | `br i1 %c, label %T, label %F` | 0: `%c`, 1: `%F`, 2: `%T` | Successors reversed vs print | `getCondition()`, `getSuccessor(0)` = true, `getSuccessor(1)` = false |
| ✅ `BranchInst` (uncond) | `br label %dst` | 0: `%dst` | — | `getSuccessor(0)` |
| ⚠️ `SwitchInst` | `switch i64 %v, label %def [ i64 0, label %bb0 ... ]` | 0: `%v`, 1: `%def`, then pairs: value, block, value, block... | Case operands are interleaved value/block pairs | `getCondition()`, `getDefaultDest()`, `case_begin()`/`case_end()` iterators, `findCaseValue()` |
| ⚠️ `IndirectBrInst` | `indirectbr ptr %addr, [ label %bb0, label %bb1 ]` | 0: `%addr`, 1+: destinations | Destinations are operands, not a separate list | `getAddress()`, `getDestination(i)`, `getNumDestinations()` |
| ⚠️ `InvokeInst` | `invoke @foo(%a, %b) to label %N unwind label %E` | 0: `%a`, 1: `%b`, ..., -3: `%E`, -2: `%N`, -1: `@foo` | Callee is last operand; successors reversed | `getCalledOperand()`, `getArgOperand(i)`, `getNormalDest()`, `getUnwindDest()` |
| ⚠️ `CallBrInst` | `callbr @foo(%a) to label %fall [label %bb1, ...]` | 0: `%a`, ..., indirect dests..., -2: `%fall`, -1: `@foo` | Same callee-last layout; fallthrough and indirect dests mixed in | `getCalledOperand()`, `getArgOperand(i)`, `getDefaultDest()`, `getIndirectDest(i)` |
| ✅ `ReturnInst` | `ret i32 %v` / `ret void` | 0: `%v` (or no operands) | — | `getReturnValue()` |
| ✅ `ResumeInst` | `resume { ptr, i32 } %val` | 0: `%val` | — | `getValue()` |
| ⚠️ `CatchSwitchInst` | `catchswitch within %parent [label %h1, ...] unwind label %u` | 0: `%parent`, 1: `%u` (or none), 2+: handlers | Unwind dest may or may not be present (unwind to caller) | `getParentPad()`, `getUnwindDest()`, `handler_begin()`/`handler_end()` |
| ✅ `CatchReturnInst` | `catchret from %pad to label %bb` | 0: `%pad`, 1: `%bb` | — | `getCatchPad()`, `getSuccessor()` |
| ✅ `CleanupReturnInst` | `cleanupret from %pad unwind label %bb` | 0: `%pad`, 1: `%bb` (optional) | — | `getCleanupPad()`, `getUnwindDest()` |
| ✅ `UnreachableInst` | `unreachable` | (none) | — | — |

## Call-like Instructions

| Instruction | Printed IR | `getOperand(i)` layout | Gotcha | Preferred API |
|---|---|---|---|---|
| ⚠️ `CallInst` | `call @foo(%a, %b)` | 0: `%a`, 1: `%b`, ..., last: `@foo` | **Callee is the last operand**, not first | `getCalledOperand()`, `getArgOperand(i)`, `arg_begin()`/`arg_end()` |
| ⚠️ `InvokeInst` | (see above) | (see above) | (see above) | (see above) |
| ⚠️ `CallBrInst` | (see above) | (see above) | (see above) | (see above) |

## Memory Instructions

| Instruction | Printed IR | `getOperand(i)` layout | Gotcha | Preferred API |
|---|---|---|---|---|
| ✅ `LoadInst` | `load i32, ptr %p` | 0: `%p` | — | `getPointerOperand()` |
| ⚠️ `StoreInst` | `store i32 %v, ptr %p` | 0: `%v`, 1: `%p` | Matches print, but opposite of `load` (where op 0 is the pointer). Easy to mix up | `getValueOperand()`, `getPointerOperand()` |
| ✅ `AllocaInst` | `alloca i32, i64 %n` | 0: `%n` (array size, if present) | — | `getArraySize()`, `getAllocatedType()` |
| ✅ `FenceInst` | `fence seq_cst` | (none) | — | `getOrdering()`, `getSyncScopeID()` |
| ⚠️ `AtomicCmpXchgInst` | `cmpxchg ptr %p, i32 %cmp, i32 %new` | 0: `%p`, 1: `%cmp`, 2: `%new` | Matches print, but returns `{ i32, i1 }` — easy to forget the success flag | `getPointerOperand()`, `getCompareOperand()`, `getNewValOperand()` |
| ✅ `AtomicRMWInst` | `atomicrmw add ptr %p, i32 %v` | 0: `%p`, 1: `%v` | — | `getPointerOperand()`, `getValOperand()`, `getOperation()` |
| ✅ `GetElementPtrInst` | `getelementptr i32, ptr %p, i64 %i, ...` | 0: `%p`, 1+: indices | — | `getPointerOperand()`, `idx_begin()`/`idx_end()`, `getNumIndices()` |

## PHI & Select

| Instruction | Printed IR | `getOperand(i)` layout | Gotcha | Preferred API |
|---|---|---|---|---|
| ⚠️ `PHINode` | `phi i32 [ %a, %bb1 ], [ %b, %bb2 ]` | 0: `%a`, 1: `%bb1`, 2: `%b`, 3: `%bb2` | **Values and blocks interleaved** — easy to get wrong with raw operands | `getIncomingValue(i)`, `getIncomingBlock(i)`, `getNumIncomingValues()` |
| ✅ `SelectInst` | `select i1 %c, i32 %t, i32 %f` | 0: `%c`, 1: `%t`, 2: `%f` | — | `getCondition()`, `getTrueValue()`, `getFalseValue()` |

## Binary & Unary

| Instruction | Printed IR | `getOperand(i)` layout | Gotcha | Preferred API |
|---|---|---|---|---|
| ✅ `BinaryOperator` | `add i32 %a, %b` | 0: `%a`, 1: `%b` | — | `getOperand(0)`, `getOperand(1)` |
| ✅ `UnaryOperator` | `fneg float %a` | 0: `%a` | — | `getOperand(0)` |

## Comparison

| Instruction | Printed IR | `getOperand(i)` layout | Gotcha | Preferred API |
|---|---|---|---|---|
| ✅ `ICmpInst` | `icmp eq i32 %a, %b` | 0: `%a`, 1: `%b` | — | `getOperand(0)`, `getOperand(1)`, `getPredicate()` |
| ✅ `FCmpInst` | `fcmp oeq float %a, %b` | 0: `%a`, 1: `%b` | — | same as `ICmpInst` |

## Cast Instructions

| Instruction | Printed IR | `getOperand(i)` layout | Gotcha | Preferred API |
|---|---|---|---|---|
| ✅ All casts (`ZExtInst`, `SExtInst`, `TruncInst`, `BitCastInst`, `PtrToIntInst`, etc.) | `zext i32 %v to i64` | 0: `%v` | — | `getOperand(0)`, `getSrcTy()`, `getDestTy()` |

## Vector Instructions

| Instruction | Printed IR | `getOperand(i)` layout | Gotcha | Preferred API |
|---|---|---|---|---|
| ✅ `ExtractElementInst` | `extractelement <4 x i32> %v, i32 %idx` | 0: `%v`, 1: `%idx` | — | `getVectorOperand()`, `getIndexOperand()` |
| ✅ `InsertElementInst` | `insertelement <4 x i32> %v, i32 %new, i32 %idx` | 0: `%v`, 1: `%new`, 2: `%idx` | — | `getOperand(0)`, `getOperand(1)`, `getOperand(2)` |
| ⚠️ `ShuffleVectorInst` | `shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 0, ...>` | 0: `%a`, 1: `%b` — mask is **not** an operand | Mask is stored separately since LLVM 11; `getOperand(2)` does **not** exist | `getShuffleMask()`, `getShuffleMaskForBitcode()`, `getMaskValue(i)` |

## Aggregate Instructions

| Instruction | Printed IR | `getOperand(i)` layout | Gotcha | Preferred API |
|---|---|---|---|---|
| ✅ `ExtractValueInst` | `extractvalue { i32, i64 } %agg, 0` | 0: `%agg` — indices are **not** operands | Indices stored separately as metadata | `getAggregateOperand()`, `getIndices()`, `idx_begin()`/`idx_end()` |
| ✅ `InsertValueInst` | `insertvalue { i32, i64 } %agg, i32 %val, 0` | 0: `%agg`, 1: `%val` — indices not operands | Same: indices stored separately | `getAggregateOperand()`, `getInsertedValueOperand()`, `getIndices()` |

## Exception Handling

| Instruction | Printed IR | `getOperand(i)` layout | Gotcha | Preferred API |
|---|---|---|---|---|
| ✅ `LandingPadInst` | `landingpad { ptr, i32 } catch ptr @TypeInfo` | Clauses are operands | — | `getClause(i)`, `getNumClauses()`, `isCatch(i)`, `isFilter(i)` |
| ✅ `CatchPadInst` | `catchpad within %cs [ptr %arg]` | 0: `%cs`, 1+: args | — | `getCatchSwitch()`, `getArgOperand(i)` |
| ✅ `CleanupPadInst` | `cleanuppad within %parent [ptr %arg]` | 0: `%parent`, 1+: args | — | `getParentPad()`, `getArgOperand(i)` |

## Miscellaneous

| Instruction | Printed IR | `getOperand(i)` layout | Gotcha | Preferred API |
|---|---|---|---|---|
| ✅ `FreezeInst` | `freeze i32 %v` | 0: `%v` | — | `getOperand(0)` |

## Summary of Major Gotchas

| Gotcha | Instructions | Root cause |
|---|---|---|
| **Successors stored in reverse** | `BranchInst`, `InvokeInst`, `CallBrInst` | Negative indexing (`Op<-1>`) for variable operand count |
| **Callee stored last** | `CallInst`, `InvokeInst`, `CallBrInst` | Same negative indexing — callee is `Op<-1>` |
| **Interleaved value/block pairs** | `PHINode` | Each incoming edge is a (value, block) pair |
| **Interleaved value/label pairs** | `SwitchInst` | Each case is a (value, dest) pair |
| **Mask is not an operand** | `ShuffleVectorInst` | Mask moved out of operand list in LLVM 11 |
| **Indices are not operands** | `ExtractValueInst`, `InsertValueInst` | Indices are stored as constant metadata |
| **Pointer operand position differs** | `LoadInst` (op 0) vs `StoreInst` (op 1) | `StoreInst` has value as op 0, pointer as op 1 |

## Golden Rule

> **Never use raw `getOperand(i)` when a named accessor exists.** The operand layout is an internal implementation detail optimized for memory layout, not human readability.