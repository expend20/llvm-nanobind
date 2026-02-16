def validate(run, exercise, helpers):
    llvm = helpers["llvm"]
    require_ir_result = helpers["require_ir_result"]

    ok, normalized, err = require_ir_result(run, exercise=exercise)
    if not ok:
        return False, err

    with llvm.create_context() as ctx:
        with ctx.parse_ir(normalized) as mod:
            counter = mod.get_global("counter")
            if counter is None:
                return False, "Missing global `@counter`."
            if counter.linkage != llvm.Linkage.Internal:
                return False, "`@counter` must have internal linkage."
            if counter.initializer is None:
                return False, "`@counter` must have initializer `i32 0`."
            if not counter.initializer.is_constant_int or counter.initializer.const_sext_value != 0:
                return False, "`@counter` initializer must be integer 0."
            if counter.initializer.type.int_width != 32:
                return False, "`@counter` initializer type must be i32."
    return True, "Pass: added internal i32 counter global."
