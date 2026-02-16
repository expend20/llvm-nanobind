import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        with ctx.parse_ir(input_ir) as mod:
            g_i32 = mod.add_global(ctx.types.i32, "g_i32")
            g_i32.initializer = ctx.types.i32.constant(7)

            g_i64 = mod.add_global(ctx.types.i64, "g_i64")
            g_i64.initializer = ctx.types.i64.constant(7)

            return mod.to_string()
