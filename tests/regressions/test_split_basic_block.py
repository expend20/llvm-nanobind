"""
Tests for BasicBlock split helpers.

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

            # Split at the mul instruction - it and ret should move.
            split_point = product
            new_bb = entry.split_basic_block(split_point, "split")

            # Original block should end with an unconditional branch.
            entry_instrs = list(entry.instructions)
            assert entry_instrs[-1].opcode_name == "br", (
                f"Expected 'br', got '{entry_instrs[-1].opcode_name}'"
            )

            # The branch target should be the new block.
            entry_term = entry.terminator
            successors = list(entry_term.successors)
            assert len(successors) == 1
            assert successors[0].name == "split"

            # New block should contain mul and ret.
            new_instrs = list(new_bb.instructions)
            opcodes = [i.opcode_name for i in new_instrs]
            assert "mul" in opcodes, f"Expected 'mul' in new block, got {opcodes}"
            assert "ret" in opcodes, f"Expected 'ret' in new block, got {opcodes}"

            # add should remain in the original block.
            entry_opcodes = [i.opcode_name for i in entry_instrs]
            assert "add" in entry_opcodes, (
                f"Expected 'add' in original block, got {entry_opcodes}"
            )

            # Module should verify.
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

            blocks_before = [bb.name for bb in fn.basic_blocks]

            # Try to split bb1 with an instruction from bb2.
            try:
                bb1.split_basic_block(val)
                assert False, "Expected an exception for wrong block"
            except Exception as e:
                assert "not found" in str(e), f"Unexpected error: {e}"

            blocks_after = [bb.name for bb in fn.basic_blocks]
            assert blocks_after == blocks_before, (
                "split_basic_block should not mutate CFG on validation failure"
            )


def test_split_basic_block_rejects_phi():
    """Splitting at a PHI node should raise to avoid invalid IR."""
    with llvm.create_context() as ctx:
        with ctx.create_module("test") as m:
            i1 = ctx.types.i1
            i32 = ctx.types.i32
            fn_ty = ctx.types.function(i32, [i1, i32, i32])
            fn = m.add_function("test_split_phi", fn_ty)

            entry = fn.append_basic_block("entry")
            left = fn.append_basic_block("left")
            right = fn.append_basic_block("right")
            merge = fn.append_basic_block("merge")

            with entry.create_builder() as builder:
                cond = fn.get_param(0)
                builder.cond_br(cond, left, right)

                builder.position_at_end(left)
                builder.br(merge)

                builder.position_at_end(right)
                builder.br(merge)

                builder.position_at_end(merge)
                phi = builder.phi(i32, "merged")
                phi.add_incoming(fn.get_param(1), left)
                phi.add_incoming(fn.get_param(2), right)
                builder.ret(phi)

            blocks_before = [bb.name for bb in fn.basic_blocks]
            try:
                merge.split_basic_block(phi)
                assert False, "Expected split_basic_block to reject PHI split point"
            except Exception as e:
                assert "PHI" in str(e), f"Unexpected error: {e}"

            blocks_after = [bb.name for bb in fn.basic_blocks]
            assert blocks_after == blocks_before, (
                "split_basic_block should not mutate CFG when rejecting PHI split"
            )
            assert m.verify(), "Module should remain valid after failed split attempt"


def test_split_basic_block_before():
    """split_basic_block_before moves prefix instructions into a new predecessor."""
    with llvm.create_context() as ctx:
        with ctx.create_module("test") as m:
            i32 = ctx.types.i32
            fn_ty = ctx.types.function(i32, [i32, i32])
            fn = m.add_function("test_split_before", fn_ty)

            entry = fn.append_basic_block("entry")
            with entry.create_builder() as builder:
                a = fn.get_param(0)
                b = fn.get_param(1)
                sum_val = builder.add(a, b, "sum")
                product = builder.mul(sum_val, b, "product")
                builder.ret(product)

            new_pred = entry.split_basic_block_before(product, "entry_prefix")

            fn_blocks = [bb.name for bb in fn.basic_blocks]
            assert fn_blocks == ["entry_prefix", "entry"], fn_blocks

            new_pred_instrs = list(new_pred.instructions)
            new_pred_ops = [inst.opcode_name for inst in new_pred_instrs]
            assert "add" in new_pred_ops, new_pred_ops
            assert "mul" not in new_pred_ops, new_pred_ops
            assert new_pred_ops[-1] == "br", new_pred_ops

            entry_instrs = list(entry.instructions)
            entry_ops = [inst.opcode_name for inst in entry_instrs]
            assert "mul" in entry_ops, entry_ops
            assert "add" not in entry_ops, entry_ops
            assert entry_ops[-1] == "ret", entry_ops

            pred_term = new_pred.terminator
            successors = list(pred_term.successors)
            assert len(successors) == 1
            assert successors[0].name == "entry"
            assert m.verify(), "Module failed verification after split_basic_block_before"


def test_split_basic_block_before_rewires_predecessors():
    """split_basic_block_before should redirect incoming edges to the new block."""
    with llvm.create_context() as ctx:
        with ctx.create_module("test") as m:
            i32 = ctx.types.i32
            fn_ty = ctx.types.function(i32, [i32])
            fn = m.add_function("test_split_before_preds", fn_ty)

            start = fn.append_basic_block("start")
            work = fn.append_basic_block("work")

            with start.create_builder() as builder:
                builder.br(work)

                builder.position_at_end(work)
                add_inst = builder.add(fn.get_param(0), i32.constant(1), "inc")
                builder.ret(add_inst)

            pre_work = work.split_basic_block_before(add_inst, "pre_work")

            start_term = start.terminator
            start_succ = list(start_term.successors)
            assert len(start_succ) == 1
            assert start_succ[0].name == "pre_work"

            pre_work_term = pre_work.terminator
            pre_work_succ = list(pre_work_term.successors)
            assert len(pre_work_succ) == 1
            assert pre_work_succ[0].name == "work"

            assert m.verify(), "Module failed verification after predecessor rewiring"


def test_split_basic_block_before_rejects_phi():
    """split_basic_block_before rejects PHI split points."""
    with llvm.create_context() as ctx:
        with ctx.create_module("test") as m:
            i1 = ctx.types.i1
            i32 = ctx.types.i32
            fn_ty = ctx.types.function(i32, [i1, i32, i32])
            fn = m.add_function("test_split_before_phi", fn_ty)

            entry = fn.append_basic_block("entry")
            left = fn.append_basic_block("left")
            right = fn.append_basic_block("right")
            merge = fn.append_basic_block("merge")

            with entry.create_builder() as builder:
                cond = fn.get_param(0)
                builder.cond_br(cond, left, right)

                builder.position_at_end(left)
                builder.br(merge)

                builder.position_at_end(right)
                builder.br(merge)

                builder.position_at_end(merge)
                phi = builder.phi(i32, "merged")
                phi.add_incoming(fn.get_param(1), left)
                phi.add_incoming(fn.get_param(2), right)
                builder.ret(phi)

            blocks_before = [bb.name for bb in fn.basic_blocks]
            try:
                merge.split_basic_block_before(phi)
                assert False, "Expected split_basic_block_before to reject PHI split point"
            except Exception as e:
                assert "PHI" in str(e), f"Unexpected error: {e}"

            blocks_after = [bb.name for bb in fn.basic_blocks]
            assert blocks_after == blocks_before, (
                "split_basic_block_before should not mutate CFG when rejecting PHI split"
            )
            assert m.verify(), "Module should remain valid after failed split attempt"


if __name__ == "__main__":
    test_split_basic_block()
    print("test_split_basic_block: PASSED")

    test_split_basic_block_wrong_instruction()
    print("test_split_basic_block_wrong_instruction: PASSED")

    test_split_basic_block_rejects_phi()
    print("test_split_basic_block_rejects_phi: PASSED")

    test_split_basic_block_before()
    print("test_split_basic_block_before: PASSED")

    test_split_basic_block_before_rewires_predecessors()
    print("test_split_basic_block_before_rewires_predecessors: PASSED")

    test_split_basic_block_before_rejects_phi()
    print("test_split_basic_block_before_rejects_phi: PASSED")
