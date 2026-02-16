# Python API Reference

This project's comprehensive API reference is the generated type stub:

- `.venv/Lib/site-packages/llvm/__init__.pyi`

That file is generated from the nanobind bindings and contains the full public
surface area (functions, classes, methods, properties, argument names, and
types) for the currently built version.

## How To Regenerate

```bash
uv sync
```

The stub is regenerated during rebuild.

## How To Browse Quickly

```bash
# List public classes
rg "^class " .venv/Lib/site-packages/llvm/__init__.pyi

# List top-level functions
rg "^def " .venv/Lib/site-packages/llvm/__init__.pyi

# Jump to one API quickly
rg "replace_all_uses_with|erase_from_parent|split_basic_block|const_string" .venv/Lib/site-packages/llvm/__init__.pyi
```

## Notes

- This is the canonical reference for exact signatures.
- Narrative usage guidance and caveats remain in:
  - `README.md`
  - `devdocs/porting-guide.md`
  - `devdocs/lit-tests.md`
  - `devdocs/DEBUGGING.md`

## Value API Validity Matrix

This section documents when `llvm.Value` accessors are valid to call. Invalid
calls should raise `llvm.LLVMAssertionError` (not crash).

### Always Valid (for any live Value)

- Generic identity/introspection:
  - `type`, `name`, `value_kind`, `is_constant`, `is_undef`, `is_poison`
- Use/operand graph:
  - `uses`, `users`, `has_uses`, `num_operands`, `operands`, `get_operand`,
    `set_operand`, `get_operand_use`
- Type predicates:
  - `is_*` predicate properties (function/global/constant/etc.)

### Global Families

- Global variable only:
  - `next_global`, `prev_global`
  - `initializer` (get/set)
  - `set_constant`, `is_global_constant`
  - `set_thread_local`, `is_thread_local`
  - `set_externally_initialized`, `is_externally_initialized`
  - `delete_global`, `delete`
- Global value:
  - `global_value_type`, `unnamed_address`
  - `linkage`, `visibility`, `dll_storage_class`
  - `global_copy_all_metadata`
- Global object:
  - `comdat`, `set_comdat`
  - `section`
  - `alignment` (global path)

### Function Families

- Function only:
  - `has_personality_fn`, `personality_fn`, `set_personality_fn`
  - `has_prefix_data`, `prefix_data`, `set_prefix_data`
  - `has_prologue_data`, `prologue_data`, `set_prologue_data`
  - `function_type`

### Instruction Families

- Any instruction:
  - `opcode`, `opcode_name`
  - `instruction_get_all_metadata_other_than_debug_loc`
  - `next_instruction`, `prev_instruction`
  - `remove_from_parent`, `erase_from_parent`, `delete_instruction`,
    `instruction_clone`
- Branch:
  - `is_conditional`, `condition` require `br`
- Terminator:
  - `num_successors`, `get_successor`, `successors`, `unwind_dest`
- PHI:
  - `num_incoming`, `get_incoming_value`, `get_incoming_block`, `incoming`
- LandingPad:
  - `num_clauses`, `get_clause`, `is_cleanup`, `set_cleanup`, `add_clause`
- CatchPad/CatchSwitch:
  - `parent_catch_switch`, `num_handlers`, `handlers`, `add_handler`
- ShuffleVector:
  - `num_mask_elements`, `get_mask_value`

### Opcode-Specific Instruction Accessors

- `icmp_predicate`: `icmp`
- `fcmp_predicate`: `fcmp`
- `nsw` / `set_nsw`: `add`, `sub`, `mul`, `shl`
- `nuw` / `set_nuw`: `add`, `sub`, `mul`, `shl`
- `exact` / `set_exact`: `udiv`, `sdiv`, `lshr`, `ashr`
- `nneg` / `set_nneg`: `zext`
- `is_disjoint` / `set_is_disjoint`: `or`
- `icmp_same_sign` / `set_icmp_same_sign`: `icmp`
- `allocated_type`: `alloca`

### Call/Invoke/CallBr Families

- Call-like (`call`, `invoke`, `callbr`):
  - `num_operand_bundles`, `get_operand_bundle_at_index`
  - `called_function_type`, `called_value`, `set_called_operand`
- Arg operand instructions (`call`, `invoke`, `callbr`, `catchpad`,
  `cleanuppad`):
  - `num_arg_operands`, `get_arg_operand`
- Tail call kind (`call`, `invoke`):
  - `tail_call_kind`, `set_tail_call_kind`
- Callsite attributes:
  - `get_callsite_attribute_count`
  - `get_callsite_enum_attribute`
  - `add_callsite_attribute`
  - Requires call-like instruction and `idx >= -1`.

### Atomic/Memory Families

- Volatile accessors:
  - `is_volatile`, `set_volatile` require `load`/`store`
- Ordering accessors:
  - `ordering`, `set_ordering` require atomic-capable memory instruction
    (`load`, `store`, `fence`, `atomicrmw`, `cmpxchg`)
- Atomic metadata:
  - `is_atomic`: instruction only
  - `atomic_sync_scope_id`, `set_atomic_sync_scope_id`: atomic-capable memory
    instruction
  - `atomic_rmw_bin_op`: `atomicrmw`
  - `cmpxchg_success_ordering`, `cmpxchg_failure_ordering`, `weak`, `set_weak`:
    `cmpxchg`

### Indexed/GEP Families

- `num_indices`:
  - `getelementptr`, `extractvalue`, `insertvalue` (instruction or const-expr)
- `indices`:
  - `extractvalue`/`insertvalue` instruction, or const-expr
    `getelementptr`/`extractvalue`/`insertvalue`
- `gep_source_element_type`, `gep_no_wrap_flags`:
  - GEP value (instruction or const-expr)

### Constant-Only / Type-Safe Mutators

- Constant-only:
  - `const_bitcast` requires a constant value
- Replacement:
  - `replace_all_uses_with` requires same context and identical type
- Fast-math:
  - `fast_math_flags`, `set_fast_math_flags` require
    `can_use_fast_math_flags == True`
