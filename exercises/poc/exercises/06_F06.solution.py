import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        saved = ctx.types.i32.constant(123)

    try:
        _ = saved.type
    except BaseException as exc:
        return type(exc).__name__

    return "NO_EXCEPTION"
