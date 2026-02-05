# Transformation API Improvements - Progress

## Status: Planning Complete

## Completed
- [x] Identified all issues during obfuscation pass porting
- [x] Documented in `devdocs/porting-guide.md`
- [x] Created prioritized plan

## In Progress
- [ ] None yet

## Not Started

### Priority 1: Critical Blockers
- [ ] 1.1 Raw bytes support for constants
- [ ] 1.2 Add `Value.replace_all_uses_with()`
- [ ] 1.3 Add `BasicBlock.split_before()`
- [ ] 1.4 Single-step instruction deletion (`erase_from_parent`)

### Priority 2: API Consistency
- [ ] 2.1 Make `ptr` a property
- [ ] 2.2 Consistent setter patterns
- [ ] 2.3 Add `.parent` alias for instructions
- [ ] 2.4 Rename `.is_terminator_inst` to `.is_terminator`

### Priority 3: Missing Conveniences
- [ ] 3.1 Add `.operands` iterator
- [ ] 3.2 Add instruction movement
- [ ] 3.3 Add instruction cloning
- [ ] 3.4 Add `.num_successors` property

### Priority 4: Documentation
- [ ] 4.1 Document exception types
- [ ] 4.2 Document context manager patterns
- [ ] 4.3 Add API reference

## Blocked
- String encryption pass in `tools/obfuscation/` - blocked by 1.1
- Full basic block splitter - blocked by 1.3

## Notes

### Discovery Process
Issues were discovered while porting the LLVM obfuscator:
- `mba_sub.py` - needed RAUW, instruction deletion
- `control_flow_flattening.py` - needed PHI access, block creation
- `simple_indirect_branch.py` - needed block address, indirect branch
- `string_encryption.py` - **abandoned** due to UTF-8 encoding bug

### Quick Verification Needed
Before implementing, verify these don't already exist:
- `LLVMInstructionEraseFromParent` binding
- `LLVMReplaceAllUsesWith` binding
- `num_successors` property

### Design Decisions Needed
- 1.1: Should `const_data_array` accept `bytes`, or add new `const_bytes` function?
- 1.3: Block splitting may need C++ wrapper - evaluate LLVM C API coverage
