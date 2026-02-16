import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        with ctx.parse_ir(input_ir) as mod:
            # TODO: add @g_i32 and @g_i64 with typed constants
            return mod.to_string()
