def validate(run, exercise, helpers):
    llvm = helpers["llvm"]
    require_ir_result = helpers["require_ir_result"]

    ok, normalized, err = require_ir_result(run, exercise=exercise)
    if not ok:
        return False, err

    with llvm.create_context() as ctx:
        with ctx.parse_ir(normalized) as mod:
            g32 = mod.get_global("g_i32")
            g64 = mod.get_global("g_i64")
            if g32 is None or g64 is None:
                return False, "Expected globals `@g_i32` and `@g_i64`."
            if g32.initializer is None or g64.initializer is None:
                return False, "Both globals must have initializers."
            if not g32.initializer.is_constant_int or g32.initializer.const_sext_value != 7:
                return False, "`@g_i32` initializer must be integer 7."
            if not g64.initializer.is_constant_int or g64.initializer.const_sext_value != 7:
                return False, "`@g_i64` initializer must be integer 7."
            if g32.initializer.type.int_width != 32:
                return False, "`@g_i32` initializer must be i32."
            if g64.initializer.type.int_width != 64:
                return False, "`@g_i64` initializer must be i64."
    return True, "Pass: created typed integer globals correctly."
