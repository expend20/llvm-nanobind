def validate(run, exercise, helpers):
    llvm = helpers["llvm"]
    require_ir_result = helpers["require_ir_result"]

    ok, normalized, err = require_ir_result(run, exercise=exercise)
    if not ok:
        return False, err

    with llvm.create_context() as ctx:
        with ctx.parse_ir(normalized) as mod:
            fn = mod.get_function("arith")
            if fn is None:
                return False, "Missing function `@arith`."
            if fn.is_declaration:
                return False, "`@arith` must have a function body."
            if fn.param_count != 3:
                return False, "`@arith` must take three i32 parameters."

            insts = fn.entry_block.instructions
            if not insts:
                return False, "`@arith` entry block has no instructions."
            if insts[-1].opcode != llvm.Opcode.Ret:
                return False, "`@arith` must end with `ret`."

            add_insts = [inst for inst in insts if inst.opcode == llvm.Opcode.Add]
            mul_insts = [inst for inst in insts if inst.opcode == llvm.Opcode.Mul]
            if not add_insts:
                return False, "Expected at least one integer `add` instruction."
            if not mul_insts:
                return False, "Expected at least one integer `mul` instruction."

            ret_op = insts[-1].get_operand(0)
            if not any(ret_op == mul for mul in mul_insts):
                return False, "Return operand should be the `mul` result."

            add_feeds_mul = False
            for mul in mul_insts:
                for add in add_insts:
                    for idx in range(mul.num_operands):
                        if mul.get_operand(idx) == add:
                            add_feeds_mul = True
                            break
                    if add_feeds_mul:
                        break
                if add_feeds_mul:
                    break

            if not add_feeds_mul:
                return False, "Expected multiplication to consume an addition result."

    return True, "Pass: built arithmetic chain `(a + b) * c`."
