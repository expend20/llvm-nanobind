# Workshop Portal POC

This is a minimal local web portal with 10 LLVM Python exercises:

- Foundations: `F01` to `F08`
- Builder basics: `B01`, `B02`

## Run

```bash
uv run python exercises/poc/portal.py
```

Open:

```text
http://127.0.0.1:8123
```

## What It Includes

- Exercise picker with prompt, hints, and fixed input IR
- Starter code editor
- One-click validation
- Optional "show solution" button
- Subprocess-based evaluation (safer for runtime crashes / `SystemExit`)
- Supports `solve(input_ir)`, `main([input_ir])`, or top-level script execution
- Validators can inspect return values and/or `stdout`/`stderr`
- Metadata-driven checks in TOML (contains/regex/exception/entrypoint)
- Declarative sidecar files per exercise:
  - `<prefix>.input.ll` (optional; omit for from-scratch exercises)
  - `<prefix>.starter.py`
  - `<prefix>.solution.py`
  - `<prefix>.validator.py` (optional)

## Files

- `exercises/poc/exercises/*.toml`: exercise metadata (references files + checks)
- `exercises/poc/exercise_bank.py`: TOML loader
- `exercises/poc/evaluator.py`: execution + metadata checks + custom validators
- `exercises/poc/portal.py`: local web server and UI
- `exercises/poc/exercises/*.validator.py`: define `validate(run, exercise, helpers)`

## Notes

- This is intentionally minimal, not production hardened.
- Validation is deterministic and workshop-focused.
- For IR tasks, validators accept either returned IR text or printed IR text
  when `allow_stdout_ir` is enabled for the exercise.
- `starter_code` and `solution_code` are loaded from:
  `<prefix>.starter.py` and `<prefix>.solution.py`.
- `input_ir` is loaded from `<prefix>.input.ll` when present; if omitted, the
  exercise has no input module and users build from scratch.
- Validator files are first-class: keep exercise-specific logic in
  `<prefix>.validator.py` instead of routing through a global name map.
- `id` is inferred from filename stem (do not set it in TOML):
  - `01_F01.toml` -> `F01`
  - `10_B02.toml` -> `B02`
- Custom validators are discovered by sidecar name only:
  - `<prefix>.validator.py`
