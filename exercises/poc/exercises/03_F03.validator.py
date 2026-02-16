def validate(run, exercise, helpers):
    llvm = helpers["llvm"]
    require_ir_result = helpers["require_ir_result"]

    ok, normalized, err = require_ir_result(run, exercise=exercise)
    if not ok:
        return False, err

    with llvm.create_context() as ctx:
        with ctx.parse_ir(normalized) as mod:
            fn = mod.get_function("answer")
            if fn is None:
                return False, "Missing function `@answer`."
            if fn.is_declaration:
                return False, "`@answer` must have a body."
            if fn.param_count != 0:
                return False, "`@answer` should take no parameters."
            term = fn.entry_block.terminator
            if term.opcode != llvm.Opcode.Ret:
                return False, "Entry block terminator must be `ret`."
            if term.num_operands != 1:
                return False, "Return must have exactly one operand."
            rv = term.get_operand(0)
            if not rv.is_constant_int or rv.const_sext_value != 42:
                return False, "Return value must be constant `i32 42`."
    return True, "Pass: built `@answer` returning 42."
