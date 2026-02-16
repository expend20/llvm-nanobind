import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        with ctx.parse_ir(input_ir) as mod:
            # TODO: create function @arith and build add/mul/ret chain.
            return mod.to_string()
