def validate(run, exercise, helpers):
    llvm = helpers["llvm"]
    require_ir_result = helpers["require_ir_result"]

    ok, normalized, err = require_ir_result(run, exercise=exercise)
    if not ok:
        return False, err

    with llvm.create_context() as ctx:
        with ctx.parse_ir(normalized) as mod:
            fn = mod.get_function("mem_roundtrip")
            if fn is None:
                return False, "Missing function `@mem_roundtrip`."
            if fn.is_declaration:
                return False, "`@mem_roundtrip` must have a function body."
            if fn.param_count != 1:
                return False, "`@mem_roundtrip` must take exactly one i32 parameter."

            insts = fn.entry_block.instructions
            if len(insts) < 4:
                return False, "Expected at least alloca/store/load/ret in entry block."

            ops = [inst.opcode for inst in insts]
            required = [llvm.Opcode.Alloca, llvm.Opcode.Store, llvm.Opcode.Load, llvm.Opcode.Ret]
            cursor = 0
            for opcode in ops:
                if opcode == required[cursor]:
                    cursor += 1
                    if cursor == len(required):
                        break
            if cursor != len(required):
                return False, "Instruction order must include alloca -> store -> load -> ret."

            load_insts = [inst for inst in insts if inst.opcode == llvm.Opcode.Load]
            ret_inst = insts[-1]
            if ret_inst.opcode != llvm.Opcode.Ret:
                return False, "Function must end with `ret`."
            ret_val = ret_inst.get_operand(0)
            if not any(ret_val == load for load in load_insts):
                return False, "Return value should come from a `load` result."

    return True, "Pass: built memory round-trip with alloca/store/load/ret."
