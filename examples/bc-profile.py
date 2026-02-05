import argparse
import llvm


def first_non_phi(block: llvm.BasicBlock):
    instr = block.first_instruction
    while instr is not None and instr.opcode == llvm.Opcode.PHI:
        instr = instr.next_instruction
    assert instr is not None, "bad"
    return instr


def main():
    parser = argparse.ArgumentParser("bc-profile")
    parser.add_argument("ir_in", help="Input LLVM IR to profile")
    parser.add_argument("ir_out", help="LLVM IR with profiling instrumentation")
    args = parser.parse_args()

    ctx = llvm.global_context()
    with open(args.ir_in) as f:
        ir_in = f.read()
    with ctx.parse_ir(ir_in) as mod:
        start_stop_ty = ctx.types.function(ctx.types.void, [])
        start_fn = mod.add_function("Start", start_stop_ty)
        stop_fn = mod.add_function("Stop", start_stop_ty)
        enter_leave_ty = ctx.types.function(ctx.types.void, [ctx.types.ptr()])
        enter_fn = mod.add_function("FunctionEnter", enter_leave_ty)
        leave_fn = mod.add_function("FunctionLeave", enter_leave_ty)
        block_ty = ctx.types.function(
            ctx.types.void, [ctx.types.ptr(), ctx.types.ptr()]
        )
        block_fn = mod.add_function("FunctionBlock", block_ty)
        call_fn = mod.add_function("FunctionCall", block_ty)

        main_fn = mod.get_function("main")
        assert main_fn is not None, "expected main function"
        assert not main_fn.is_declaration, "main needs a body"
        main_entry = main_fn.entry_block
        assert main_entry is not None, "TODO: needs fixing"

        print(f"start.type: {start_fn.type}, function_type: {start_fn.function_type}")

        with first_non_phi(main_entry).create_builder() as builder:
            # TODO: if you pass a name here LLVM will assert (should be graceful)
            builder.call(start_fn.function_type, start_fn, [])

        for main_block in main_fn.basic_blocks:
            terminator = main_block.terminator
            if terminator.opcode == llvm.Opcode.Ret:
                print(f"return block: '{main_block.name}'") # TODO: how do we print the number if there is no name?
                print(main_block) # TODO: this should print the block bitcode?
                with terminator.create_builder() as builder:
                    builder.call(stop_fn.function_type, stop_fn, [])

        for function in mod.functions:
            if function.name == "main":
                print("skipping main")
                continue
            # TODO: is_ is not a good prefix for vscode discoverability
            if function.is_declaration:
                print(f"skipping declaration: {function.name}")
                continue
            print(f"instrumenting function: {function.name}")
            name_const = llvm.const_string(ctx, function.name)
            # TODO: I guess we need to promote this to a pointer?
            print(f"{name_const.value_kind=}")
            # TODO: should this be optional at all, maybe throw instead?
            entry_block = function.entry_block
            print("instrumenting entry block")
            assert entry_block is not None, "no entry block (bad)"
            with first_non_phi(entry_block).create_builder() as builder:
                # TODO: varargs instead?
                # TODO: this hard crashes
                builder.call(enter_fn.function_type, enter_fn, [name_const])
            

        with open(args.ir_out, "w", encoding="utf-8") as f:
            f.write(str(mod))


if __name__ == "__main__":
    main()
