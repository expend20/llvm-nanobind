# Agent Guidelines for llvm-nanobind

## Build & Test Commands
```bash
cmake -B build -G Ninja          # Configure (first time only)
cmake --build build              # Build everything
cmake --build build --target llvm # Build Python bindings only
python3 run_tests.py             # Run all C++ tests + Python comparison
./build/test_factorial           # Run single C++ test
python3 test_factorial.py        # Run single Python test
```

## Code Style
- **C++**: Use `LLVMXxxWrapper` for wrapper structs, `m_` prefix for members, `snake_case` for methods
- **Python**: Follow PEP 8, use `snake_case`, type hints optional but encouraged
- **Formatting**: 2-space indent in C++, 4-space in Python; no trailing whitespace
- **Imports**: C++ standard headers first, then LLVM-C headers, then nanobind
- **Error handling**: Throw `LLVMException` subclasses in C++; call `check_valid()` before LLVM-C API calls

## Testing Pattern (Golden Master)
C++ tests output LLVM IR to stdout → saved as `tests/output/*.ll` → Python tests must produce identical output.
Tests must be deterministic (no timestamps, PIDs, addresses). See `devdocs/plan.md` for templates.
