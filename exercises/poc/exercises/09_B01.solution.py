import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        with ctx.parse_ir(input_ir) as mod:
            fn_ty = ctx.types.function(
                ctx.types.i32, [ctx.types.i32, ctx.types.i32, ctx.types.i32]
            )
            fn = mod.add_function("arith", fn_ty)
            a = fn.get_param(0)
            b = fn.get_param(1)
            c = fn.get_param(2)

            entry = fn.append_basic_block("entry")
            with entry.create_builder() as builder:
                ab = builder.add(a, b, "ab")
                result = builder.mul(ab, c, "result")
                builder.ret(result)
            return mod.to_string()
