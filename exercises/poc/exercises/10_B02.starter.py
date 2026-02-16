import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        with ctx.parse_ir(input_ir) as mod:
            # TODO: create function @mem_roundtrip with alloca/store/load/ret.
            return mod.to_string()
