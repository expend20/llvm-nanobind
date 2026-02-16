import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        with ctx.parse_ir(input_ir) as mod:
            fn_ty = ctx.types.function(
                ctx.types.i32, [ctx.types.i32, ctx.types.i32]
            )
            mod.add_function("sum2", fn_ty)
            return mod.to_string()
