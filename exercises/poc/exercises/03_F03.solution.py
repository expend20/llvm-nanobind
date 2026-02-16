import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        with ctx.parse_ir(input_ir) as mod:
            fn_ty = ctx.types.function(ctx.types.i32, [])
            fn = mod.add_function("answer", fn_ty)
            entry = fn.append_basic_block("entry")
            with entry.create_builder() as builder:
                builder.ret(ctx.types.i32.constant(42))
            return mod.to_string()
