import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        with ctx.parse_ir(input_ir) as mod:
            # TODO: add declare i32 @sum2(i32, i32)
            return mod.to_string()
