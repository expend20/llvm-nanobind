import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        mgr = ctx.create_module("m")
        try:
            mgr.dispose()
        except BaseException as exc:
            return type(exc).__name__
    return "NO_EXCEPTION"
