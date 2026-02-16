# Design Note: `BasicBlock.create_builder` and `first_non_phi`

## Context

In LLVM C++, common insertion code often looks like:

```cpp
IRBuilder<> B(BB->getFirstNonPHI());
```

`getFirstNonPHI()` returns a block iterator; for empty/PHI-only blocks it points
to `BB->end()`, which naturally means "insert at end".

In Python bindings, `first_non_phi` currently returns `None` in that case, so
the exact C++ style does not map 1:1.

## Design Problem

We want:

1. Happy-path API ergonomics for insertion before first non-PHI.
2. Minimal `None` handling in user code.
3. Clear semantics (avoid ambiguous `None` meaning).

## Options

### Option A: Keep separate helper (legacy shape)

```python
with bb.create_builder_before_first_non_phi() as b:
    b.call(hook, [])
```

Pros:
- Explicit intent.
- No optional handling by caller.

Cons:
- Slight API surface growth.
- Nearby concepts split across two methods.

### Option B: Fold into `create_builder` keyword

```python
with bb.create_builder(first_non_phi=True) as b:
    b.call(hook, [])
```

Semantics:
- `first_non_phi=False` (default): position at end (current behavior).
- `first_non_phi=True`: position before first non-PHI, or end if none exists.

Pros:
- Most concise and discoverable.
- Keeps one primary entrypoint for builder placement.

Cons:
- Requires keyword design discipline to avoid overload ambiguity later.

### Option C: Accept optional instruction argument

```python
with bb.create_builder(insert_before=bb.first_non_phi) as b:
    ...
```

Semantics:
- `insert_before=None` means end.

Pros:
- Mirrors C++ "iterator may be end" concept.

Cons:
- Reintroduces optional plumbing in user code.
- Easier to hide accidental `None` bugs.

## Chosen Direction

Option B:

`BasicBlock.create_builder(*, first_non_phi: bool = False)`

Rationale:
- Happy-path first.
- Lowest friction for real usage.
- Avoids passing `None` as control flow.

## Migration Notes

- Use keyword form in examples and user-facing docs:

```python
with entry_block.create_builder(first_non_phi=True) as b:
    b.call(enter_fn, [name_const])
```
