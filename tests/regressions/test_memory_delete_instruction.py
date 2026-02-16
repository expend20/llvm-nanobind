"""
Regression for parented Value.delete_instruction() heap corruption.

Historical failure:
- Calling `Value.delete_instruction()` on a parented instruction delegated
  directly to `LLVMDeleteInstruction`, which could corrupt heap state and crash
  process teardown on Windows (`0xc0000374`).

Current contract:
- Parented instruction deletion must use erase-from-parent semantics first.
- The Python call must complete without hard crash.
- The deleted wrapper must be invalidated and raise LLVMMemoryError on reuse.
"""

import llvm


def test_delete_instruction_parented_is_safe():
    with llvm.create_context() as ctx:
        with ctx.create_module("delete_instruction_regression") as mod:
            i32 = ctx.types.i32
            fty = ctx.types.function(ctx.types.void, [], False)
            fn = mod.add_function("f", fty)
            bb = fn.append_basic_block("entry")

            with bb.create_builder() as b:
                slot = b.alloca(i32, "slot")
                b.store(i32.constant(1, False), slot)
                lhs = b.load(i32, slot, "lhs")
                inst = b.add(lhs, i32.constant(2, False), "sum")
                b.ret_void()

            inst.delete_instruction()

            try:
                _ = inst.name
                assert False, "Expected deleted instruction wrapper to be invalid"
            except llvm.LLVMMemoryError:
                pass


if __name__ == "__main__":
    test_delete_instruction_parented_is_safe()
    print("test_delete_instruction_parented_is_safe: PASSED")
