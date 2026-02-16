import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        with ctx.parse_ir(input_ir) as mod:
            counter = mod.add_global(ctx.types.i32, "counter")
            counter.initializer = ctx.types.i32.constant(0)
            counter.linkage = llvm.Linkage.Internal
            return mod.to_string()
