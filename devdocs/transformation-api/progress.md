# Transformation API Improvements - Progress

## Status: Complete (API evolved during implementation)

This file tracks progress against the **original** plan in `plan.md`.
Some completed items were shipped under different names/designs.

## Completed
- [x] Identified all issues during obfuscation pass porting
- [x] Documented in `devdocs/porting-guide.md`
- [x] Created prioritized plan

### Priority 1: Critical Blockers
- [x] 1.1 Raw bytes support for constants (`const_string`/`const_data_array` accept `bytes`)
- [x] 1.2 Add `Value.replace_all_uses_with()`
- [x] 1.3 Add basic block splitting (`split_basic_block` / `split_basic_block_before`)
- [x] 1.4 Single-step instruction deletion (`erase_from_parent`)

### Priority 2: API Consistency
- [x] 2.1 Make `ptr` a property (`ctx.types.ptr`)
- [x] 2.2 Consistent setter patterns (property path exists via `is_global_constant`)
- [x] 2.3 Add `.parent` alias for instructions
- [x] 2.4 Rename/add alias from `.is_terminator_inst` to `.is_terminator`

### Priority 3: Missing Conveniences
- [x] 3.1 Add `.operands` property
- [x] 3.2 Add instruction movement methods (`inst.move_before` / `inst.move_after`)
- [x] 3.3 Add instruction cloning (`instruction_clone`)
- [x] 3.4 Add `.num_successors` property

### Priority 4: Documentation
- [x] 4.1 Document exception types (in `devdocs/porting-guide.md`)
- [x] 4.2 Document context manager patterns (in `devdocs/porting-guide.md`)
- [x] 4.3 Add comprehensive API reference location (`devdocs/api-reference.md` + generated `.pyi`)

## Not Done At All (Original Plan Items Still Open)

- [ ] None

## Blocked
- None

## Notes

### Naming/Design Evolution
- Original `split_before()` became `split_basic_block_before()` (and `split_basic_block()`).
- Original `clone()` concept shipped as `instruction_clone()`.
- Setter consistency evolved to include a property path (`is_global_constant`) while keeping method compatibility.
- `is_terminator_inst` was replaced with `is_terminator` (no backward-compat alias by project policy).

### Discovery Process
Issues were discovered while porting the LLVM obfuscator:
- `mba_sub.py` - needed RAUW, instruction deletion
- `control_flow_flattening.py` - needed PHI access, block creation
- `simple_indirect_branch.py` - needed block address, indirect branch
- `string_encryption.py` - was previously blocked by UTF-8 string encoding behavior
