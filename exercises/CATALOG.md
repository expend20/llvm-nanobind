# Exercise Catalog (Workshop Draft)

This catalog is ordered by teaching priority. Each exercise is intended to run in an interactive portal with fixed input IR and deterministic checks.

## Track 1: Foundations

1. `F01` Parse and Print
   - Focus: `Context.parse_ir`, module printing
   - Pitfall: parse errors and diagnostic handling
   - Validation: output IR round-trip + `verify`
2. `F02` Add a Function Signature
   - Focus: `TypeFactory.function`, `Module.add_function`
   - Pitfall: varargs and return type mismatch
   - Validation: function signature check
3. `F03` Create Entry Block and Return
   - Focus: `append_basic_block`, `Builder.ret`
   - Pitfall: missing terminator
   - Validation: block terminator property
4. `F04` Integer Constants and Types
   - Focus: integer types, constants
   - Pitfall: width mismatch (`i32` vs `i64`)
   - Validation: constant type + operand type checks
5. `F05` Verify Broken IR
   - Focus: `verify`, `get_verification_error`
   - Pitfall: assuming parse success implies valid IR
   - Validation: expected verifier failure message category
6. `F06` Context Lifetime Guard
   - Focus: context manager semantics
   - Pitfall: use-after-context-destroyed
   - Validation: raises `LLVMMemoryError`
7. `F07` Module Manager Misuse
   - Focus: enter/exit/dispose behavior
   - Pitfall: double enter/double dispose
   - Validation: expected exception class
8. `F08` Global Variable Basics
   - Focus: `add_global`, initializer, linkage
   - Pitfall: invalid initializer type
   - Validation: global properties + verifier

## Track 2: Builder Fundamentals

1. `B01` Arithmetic Chain
   - Focus: `add`, `sub`, `mul`
   - Pitfall: signed/unsigned assumptions
   - Validation: exact instruction opcode sequence
2. `B02` Memory Roundtrip
   - Focus: `alloca`, `store`, `load`
   - Pitfall: loading wrong pointee type
   - Validation: verifier + type checks
3. `B03` Pointer Arithmetic with GEP
   - Focus: `gep2`, `struct_gep`
   - Pitfall: wrong index types/order
   - Validation: resulting pointer element type
4. `B04` Integer Cast Decision
   - Focus: `trunc`, `zext`, `sext`, `get_cast_opcode`
   - Pitfall: sign extension mistakes
   - Validation: expected cast opcode
5. `B05` Compare and Select
   - Focus: `icmp`, `select`
   - Pitfall: predicate/type mismatch
   - Validation: predicate + result type
6. `B06` Floating Compare
   - Focus: `fcmp`, fast-math awareness
   - Pitfall: ordered vs unordered comparisons
   - Validation: expected predicate
7. `B07` Call Builder Basics
   - Focus: `call`, callee type rules
   - Pitfall: argument count/type mismatch
   - Validation: verifier + call signature properties
8. `B08` Insertion Point Recovery
   - Focus: `position_at_end`, `clear_insertion_position`
   - Pitfall: emitting with no insertion point
   - Validation: expected exception on bad path

## Track 3: Control Flow and SSA

1. `S01` If/Else Diamond
   - Focus: `cond_br`, merge block
   - Pitfall: missing merge terminator
   - Validation: CFG shape check
2. `S02` PHI in Diamond
   - Focus: `phi`, `add_incoming`
   - Pitfall: missing predecessor edge
   - Validation: incoming count equals predecessor count
3. `S03` Counting Loop
   - Focus: loop header/latch
   - Pitfall: wrong backedge condition
   - Validation: expected loop trip semantics on constants
4. `S04` Loop-Carried PHI
   - Focus: multi-incoming PHI correctness
   - Pitfall: incoming block/value mismatch
   - Validation: verifier + phi incoming pair checks
5. `S05` Switch Construction
   - Focus: `switch`, default + cases
   - Pitfall: duplicate case values
   - Validation: case table properties
6. `S06` Split Basic Block Safely
   - Focus: `split_basic_block`, `split_basic_block_before`
   - Pitfall: splitting at invalid instruction (PHI/wrong block)
   - Validation: expected exception class
7. `S07` Predecessor Introspection
   - Focus: `predecessors`, `successors`
   - Pitfall: assuming ordering guarantees
   - Validation: set comparison (order-independent)
8. `S08` Exception Edges Intro
   - Focus: `invoke`, unwind destination basics
   - Pitfall: invalid unwind targets
   - Validation: verifier + unwind edge checks

## Track 4: Module, Attributes, Metadata

1. `M01` Function Attributes
   - Focus: enum/string attributes
   - Pitfall: wrong attribute index (`-1`, return, param)
   - Validation: attribute presence query
2. `M02` Callsite Attributes
   - Focus: call/invoke attribute APIs
   - Pitfall: non-call instruction misuse
   - Validation: expected assertion path
3. `M03` Linkage and Visibility
   - Focus: linkage, visibility, DLL storage class
   - Pitfall: incompatible combinations
   - Validation: printed IR property checks
4. `M04` Named Metadata
   - Focus: module named metadata nodes
   - Pitfall: wrong operand kind conversion
   - Validation: metadata node count
5. `M05` Instruction Metadata
   - Focus: set/get metadata on instructions
   - Pitfall: MD kind ID confusion across contexts
   - Validation: metadata round-trip check
6. `M06` Clone and Link Modules
   - Focus: `clone`, `link_module`
   - Pitfall: symbol conflicts and ownership assumptions
   - Validation: linked symbol table checks

## Track 5: Optimization and Target

1. `P01` Run `instcombine`
   - Focus: `run_passes`
   - Pitfall: expecting optimization without passes
   - Validation: before/after IR delta
2. `P02` Custom PassBuilderOptions
   - Focus: option toggles
   - Pitfall: assuming all options affect all pipelines
   - Validation: pipeline runs and IR remains valid
3. `P03` Target Triple and Data Layout
   - Focus: target discovery and assignment
   - Pitfall: mismatched triple/data layout
   - Validation: module fields + verifier
4. `P04` Emit Object to Memory Buffer
   - Focus: target machine codegen
   - Pitfall: missing target initialization
   - Validation: non-empty object bytes
5. `P05` Parse Object and List Symbols
   - Focus: binary iterators
   - Pitfall: iterator lifetime/order misuse
   - Validation: stable symbol list properties
6. `P06` Disassemble Bytes
   - Focus: disassembly context
   - Pitfall: wrong triple/cpu/features
   - Validation: expected mnemonic subset

## Track 6: Debug Info (Optional Advanced)

1. `D01` Minimal Compile Unit
   - Focus: `DIBuilder.create_compile_unit`
   - Pitfall: forgetting `finalize`
   - Validation: debug metadata present
2. `D02` Function Debug Info
   - Focus: subprogram metadata + `set_subprogram`
   - Pitfall: dangling debug references
   - Validation: function has `!dbg`
3. `D03` Source Location Records
   - Focus: debug location helpers
   - Pitfall: invalid scope/inlined-at chain
   - Validation: line/column/scope accessors
4. `D04` Debug Metadata Inspection
   - Focus: helper getters (`get_di_node_tag`, etc.)
   - Pitfall: wrong metadata kind usage
   - Validation: tag/name/line checks

## Capstones

1. `C01` Dead Branch Simplifier
   - Task: fold constant branch conditions and clean CFG
   - Validation: CFG property checks + verifier
2. `C02` Instrument Function Entries
   - Task: inject profiling call at first non-PHI
   - Validation: instruction placement correctness
3. `C03` Add Bounds Checks
   - Task: guard memory accesses with conditional trap
   - Validation: expected control-flow skeleton
4. `C04` IR Linter Challenge
   - Task: detect and report high-risk constructs
   - Validation: expected diagnostics list

