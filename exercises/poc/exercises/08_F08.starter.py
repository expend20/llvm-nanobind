import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        with ctx.parse_ir(input_ir) as mod:
            # TODO: add @counter, set initializer to i32 0, set linkage to Internal
            return mod.to_string()
