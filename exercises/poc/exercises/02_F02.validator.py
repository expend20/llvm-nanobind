def validate(run, exercise, helpers):
    llvm = helpers["llvm"]
    require_ir_result = helpers["require_ir_result"]

    ok, normalized, err = require_ir_result(run, exercise=exercise)
    if not ok:
        return False, err

    with llvm.create_context() as ctx:
        with ctx.parse_ir(normalized) as mod:
            fn = mod.get_function("sum2")
            if fn is None:
                return False, "Missing function declaration `@sum2`."
            if not fn.is_declaration:
                return False, "`@sum2` should be a declaration (no body)."
            if fn.param_count != 2:
                return False, "`@sum2` must have exactly 2 parameters."
            p0 = fn.get_param(0)
            p1 = fn.get_param(1)
            if p0.type.kind != llvm.TypeKind.Integer or p0.type.int_width != 32:
                return False, "Parameter 0 must be i32."
            if p1.type.kind != llvm.TypeKind.Integer or p1.type.int_width != 32:
                return False, "Parameter 1 must be i32."

    if "declare i32 @sum2(i32, i32)" not in normalized:
        return False, "Return type/signature text does not match `declare i32 @sum2(i32, i32)`."
    return True, "Pass: added correct declaration `sum2(i32, i32) -> i32`."
