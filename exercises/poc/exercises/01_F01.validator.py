def validate(run, exercise, helpers):
    require_ir_result = helpers["require_ir_result"]
    normalize_ir = helpers["normalize_ir"]

    ok, normalized, err = require_ir_result(run, exercise=exercise)
    if not ok:
        return False, err

    expected = normalize_ir(str(exercise.get("input_ir", "")))
    if normalized != expected:
        return False, "Output does not match canonical round-trip of the input module."
    return True, "Pass: parsed and round-tripped input IR correctly."
