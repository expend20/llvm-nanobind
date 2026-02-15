"""
Test for BasicBlock.split_basic_block().

Verifies that splitting a basic block at a given instruction correctly
moves instructions to a new block, inserts an unconditional branch,
and produces a valid module.
"""

import llvm


def test_split_basic_block():
    """Splitting a block moves instructions and inserts a branch."""
    with llvm.create_context() as ctx:
        with ctx.create_module("test") as m:
            i32 = ctx.types.i32
            fn_ty = ctx.types.function(i32, [i32, i32])
            fn = m.add_function("test_split", fn_ty)

            entry = fn.append_basic_block("entry")
            with entry.create_builder() as builder:
                a = fn.get_param(0)
                b = fn.get_param(1)
                sum_val = builder.add(a, b, "sum")
                product = builder.mul(sum_val, b, "product")
                builder.ret(product)

            # Split at the mul instruction — it and ret should move
            split_point = product
            new_bb = entry.split_basic_block(split_point, "split")

            # Original block should end with an unconditional branch
            entry_instrs = list(entry.instructions)
            assert entry_instrs[-1].opcode_name == "br", (
                f"Expected 'br', got '{entry_instrs[-1].opcode_name}'"
            )

            # The branch target should be the new block
            entry_term = entry.terminator
            successors = list(entry_term.successors)
            assert len(successors) == 1
            assert successors[0].name == "split"

            # New block should contain mul and ret
            new_instrs = list(new_bb.instructions)
            opcodes = [i.opcode_name for i in new_instrs]
            assert "mul" in opcodes, f"Expected 'mul' in new block, got {opcodes}"
            assert "ret" in opcodes, f"Expected 'ret' in new block, got {opcodes}"

            # add should remain in the original block
            entry_opcodes = [i.opcode_name for i in entry_instrs]
            assert "add" in entry_opcodes, (
                f"Expected 'add' in original block, got {entry_opcodes}"
            )

            # Module should verify
            assert m.verify(), "Module failed verification after split"


def test_split_basic_block_wrong_instruction():
    """Splitting with an instruction not in the block should raise."""
    with llvm.create_context() as ctx:
        with ctx.create_module("test") as m:
            i32 = ctx.types.i32
            fn_ty = ctx.types.function(i32, [i32])
            fn = m.add_function("test_split_err", fn_ty)

            bb1 = fn.append_basic_block("bb1")
            bb2 = fn.append_basic_block("bb2")

            with bb1.create_builder() as builder:
                builder.br(bb2)

                builder.position_at_end(bb2)
                val = builder.add(fn.get_param(0), i32.constant(1), "val")
                builder.ret(val)

            # Try to split bb1 with an instruction from bb2
            try:
                bb1.split_basic_block(val)
                assert False, "Expected an exception for wrong block"
            except Exception as e:
                assert "not found" in str(e), f"Unexpected error: {e}"


if __name__ == "__main__":
    test_split_basic_block()
    print("test_split_basic_block: PASSED")

    test_split_basic_block_wrong_instruction()
    print("test_split_basic_block_wrong_instruction: PASSED")
