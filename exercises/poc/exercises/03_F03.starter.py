import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        with ctx.parse_ir(input_ir) as mod:
            # TODO: create function @answer and return i32 42
            return mod.to_string()
