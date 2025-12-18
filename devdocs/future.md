# Future Work

Items explicitly deferred from the current review cycle. These are not blocking end-user readiness but would improve the project.

## Out of Scope (Current Review)

### JIT Execution Examples
**Status**: Not supported in current bindings

The LLVM-C API includes JIT compilation via `ExecutionEngine.h` and `LLJIT.h`, but these are not currently bound in llvm-nanobind. Adding JIT support would require:

1. Binding `LLVMCreateExecutionEngineForModule` / `LLVMCreateMCJITCompilerForModule`
2. Binding `LLVMRunFunction` / `LLVMGetFunctionAddress`
3. Memory management for JIT-compiled code
4. Error handling for compilation failures

**When to add**: After core bindings are stable and coverage reaches 90%+.

### CI/CD Pipeline
**Status**: Deferred

A GitHub Actions workflow would provide:
- Automated test runs on PR
- Coverage reports
- Type checking enforcement
- Multi-platform testing (Linux, macOS, Windows)

**Current workaround**: Run tests locally with `uv run run_tests.py` and `uv run run_llvm_c_tests.py`.

**When to add**: Before public release / package publishing.

### Generated API Documentation
**Status**: Deferred

Options considered:
- **pdoc** - Auto-generate from docstrings
- **sphinx** - More control, requires manual RST
- **mkdocs** - Markdown-based, good for tutorials

**Current workaround**: The auto-generated `.pyi` stub file provides IDE intellisense and type information. Located at:
```
.venv/lib/python3.*/site-packages/llvm/__init__.pyi
```

**When to add**: When the API stabilizes. Generated docs from unstable APIs cause more confusion than they solve.

### Comprehensive Contributor Documentation
**Status**: Deferred

`AGENTS.md` provides AI agent guidelines but human contributors would benefit from:
- Architecture overview
- How to add new bindings
- How to debug crashes
- Code style guide

**Current workaround**: `devdocs/DEBUGGING.md` and `devdocs/memory-model.md` cover the critical topics.

**When to add**: When seeking external contributors.

---

## Known Limitations

### Switch/IndirectBr Instructions in echo.py
**Status**: Parity with upstream

The `--echo` command in llvm-c-test does not fully clone Switch or IndirectBr instructions. This is also true of the upstream C implementation:

```cpp
// From llvm-c/llvm-c-test/echo.cpp
case LLVMSwitch:
case LLVMIndirectBr:
    break;
```

**Impact**: Modules containing switch statements or computed gotos will not round-trip correctly through `--echo`.

**When to fix**: Would require upstream contribution to llvm-c-test first, then we can match.

### mypy Configuration
**Status**: Works with manual configuration

mypy doesn't automatically discover the installed stubs without explicit configuration:
```toml
[tool.mypy]
mypy_path = ".venv/lib/python3.*/site-packages"
```

**Workaround**: Use `uvx ty check` (recommended) or `uvx pyright` instead.

---

## Potential Future Features

### Optimization Passes (PassBuilder)
The LLVM-C API includes `Transforms/PassBuilder.h` for running optimization pipelines. Bindings could enable:
```python
# Hypothetical API
with llvm.PassBuilder() as pb:
    pb.parse_pipeline("default<O2>")
    pb.run(module)
```

### Module Linking
`LLVMLinker.h` provides module linking capabilities for combining multiple modules.

### Remarks/Diagnostics
`Remarks.h` provides structured optimization remarks that could be exposed for analysis tools.

### Code Generation
`TargetMachine.h` provides `LLVMTargetMachineEmitToMemoryBuffer` for generating assembly or object code. Currently partially bound but not fully tested.
