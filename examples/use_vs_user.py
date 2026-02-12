import llvm


def main():
    ctx = llvm.global_context()
    with ctx.create_module("use_vs_user") as mod:
        func_ty = ctx.types.function(ctx.types.i32, [ctx.types.i32])
        # TODO: should we name this parameter func_type or even function_type instead?
        func = mod.add_function("math", func_ty)
        entry = func.append_basic_block("entry")

        with entry.create_builder() as builder:
            # TODO: shouldn't we derive the sign from the python integer?
            c_42 = ctx.types.i32.constant(42)
            add = builder.add(func.get_param(0), c_42, name="add")
            zero = builder.xor(add, add, name="zero")
            builder.ret(zero)
            print(func)
            # The add instruction is used in the xor instruction twice, so we get 2 uses
            # Each use knows its operand index within the user instruction
            print("Uses of add:")
            for i, use in enumerate(add.uses):
                print(f"[{i}] operand {use.operand_index} of {use.user}")
            # But only one user, the xor instruction itself
            print("Users of add:")
            for i, user in enumerate(add.users):
                print(f"[{i}] User: {user}")


if __name__ == "__main__":
    main()
