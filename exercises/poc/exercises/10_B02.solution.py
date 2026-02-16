import llvm


def solve(input_ir: str) -> str:
    with llvm.create_context() as ctx:
        with ctx.parse_ir(input_ir) as mod:
            fn_ty = ctx.types.function(ctx.types.i32, [ctx.types.i32])
            fn = mod.add_function("mem_roundtrip", fn_ty)
            x = fn.get_param(0)

            entry = fn.append_basic_block("entry")
            with entry.create_builder() as builder:
                slot = builder.alloca(ctx.types.i32, "slot")
                builder.store(x, slot)
                loaded = builder.load(ctx.types.i32, slot, "loaded")
                builder.ret(loaded)
            return mod.to_string()
