# @api Documentation

## Summary

Added `@api LLVMFunctionName` references to Python binding docstrings, enabling users to easily find the underlying LLVM-C API documentation.

**Final coverage: 723/745 bindings (97%)**

## Format Used

```cpp
// Single API - with <sub> HTML tag for de-emphasized rendering
.def("method", &Wrapper::method, "arg"_a,
     R"(Description of the method.

<sub>C API: LLVMFunctionName</sub>)")

// Multiple APIs (e.g., read/write properties)
.def_prop_rw("property", &Wrapper::get, &Wrapper::set,
             R"(Description.

<sub>C API: LLVMGetProperty, LLVMSetProperty</sub>)")

// With parameter documentation
.def("build_add", &Builder::add, "lhs"_a, "rhs"_a, "name"_a = "",
     R"(Build an add instruction.

Args:
    lhs: The left-hand side value.
    rhs: The right-hand side value.
    name: The name of the resulting value (defaults to unnamed).

<sub>C API: LLVMBuildAdd</sub>)")
```

**Format rationale:**
- `<sub>` HTML tag renders the API reference smaller/de-emphasized in HTML-aware editors (VSCode, PyCharm)
- `C API:` prefix makes it explicit what the reference is for
- Double newline separates it from the main description
- Still fully searchable and visible in `help()` output

## Key Learnings

### 1. Documentation Quality Guidelines

**Good docstrings:**
- Explain what the method does, not just restate the name
- Add context for jargon terms (GEP, ptrauth, SCEV, IFunc, etc.)
- Include Args/Returns for non-obvious methods
- Keep simple properties brief but clear

**Bad patterns to avoid:**
- `"Get intrinsic ID"` - just restates the name, no value added
- `"Check if value as metadata"` - grammatically broken
- `"Get ptrauth pointer"` - unexplained jargon
- Removing existing Args/Returns/Examples sections

**Trivial properties are OK to be brief:**
- `"Block name"`, `"Next block"`, `"Build add"` - self-explanatory

### 2. Jargon That Needs Explanation

| Term | Explanation |
|------|-------------|
| GEP | GetElementPtr - instruction for pointer arithmetic |
| ptrauth | Pointer authentication (ARM64e security feature) |
| SCEV | Scalar Evolution analysis |
| IFunc | Indirect function (GNU extension) |
| ISel | Instruction selection |
| RMW | Read-modify-write (atomic operation) |
| cmpxchg | Compare-exchange (atomic operation) |
| LICM | Loop-invariant code motion |
| SLP | Superword-level parallelism |

### 3. Excluded Bindings (~22)

These don't need `@api` docs:
- Python protocols: `__eq__`, `__ne__`, `__hash__`, `__str__`, `__repr__`, `__iter__`, `__next__`, `__enter__`, `__exit__`, `__len__`
- Manager methods: `dispose()`, `is_valid`
- Pythonic wrappers that combine multiple C APIs

### 4. IDE Tooltip Behavior & Format Evolution

**Final format (Dec 2024):** Use `<sub>` HTML tag for de-emphasized rendering

```python
"""Check if this is a void type.

<sub>C API: LLVMGetTypeKind</sub>"""
```

**Why `<sub>` tag:**
- HTML-aware editors (VSCode, PyCharm) render it smaller/de-emphasized
- Still fully visible and searchable in docstrings
- More explicit than bare `@api` tag (says "C API:" prefix)
- Works well with parameter documentation sections

**Evolution of the format:**

1. **Initial:** `@api LLVMFunction` on same line → cluttered tooltips
2. **Improvement:** Double newline before `@api` → VSCode still showed full docstring
3. **Final:** `<sub>C API: LLVMFunction</sub>` → visually de-emphasized

**Alternatives considered but rejected:**
- Sphinx directives (`.. seealso::`): No better rendering in IDEs
- HTML comments (`<!-- -->`): Hidden in `help()` output too
- External mapping file: Not visible in docstrings at all
- Custom attributes: Can't be added to methods in nanobind

### 5. Verification Commands

```bash
# Check coverage
grep -c "@api LLV" src/llvm-nanobind.cpp

# Count total bindings  
grep -c '\.def\|\.def_prop' src/llvm-nanobind.cpp

# Run tests
uv run run_tests.py

# Build check
uv run python -c "import llvm; print('OK')"
```

## Cleanup & Format Changes (Dec 2024)

### Phase 1: Formatting cleanup
1. **Moved @api to separate lines** (~694 docstrings):
   - Before: `R"(Description. @api LLVMFunction)"`
   - After: `R"(Description.\n\n@api LLVMFunction)"`
   - Goal: Separate API references from descriptions

2. **Removed redundant C++ comments** (28 instances):
   - Removed standalone `/// @api` comments from C++ wrapper methods
   - These served no purpose since Python bindings already have references in docstrings

### Phase 2: Switch to `<sub>` HTML tag
After testing, VSCode showed the full docstring including `@api` in tooltips. Switched to `<sub>` tag for better UX:

- **Changed:** `@api LLVMFunction` → `<sub>C API: LLVMFunction</sub>`
- **Benefit:** HTML-aware editors render it smaller/de-emphasized
- **Coverage:** All 694 API references updated
- **Tests:** All tests still pass ✓

## Classes Documented

| Class | Bindings | Notes |
|-------|----------|-------|
| Type | ~40 | Type queries, constants, factory methods |
| Value | ~120 | Core properties, operands, flags, atomics |
| Builder | ~75 | All instruction builders |
| Module | ~35 | Properties, functions, globals, metadata |
| Function | ~25 | Parameters, attributes, blocks |
| BasicBlock | ~15 | Navigation, terminators |
| Context | ~15 | Parsing, type factory, attributes |
| TypeFactory | ~25 | Type creation methods |
| DIBuilder | ~55 | Debug info creation |
| Target/TargetMachine/TargetData | ~30 | Target queries, code gen |
| Attribute | ~10 | Kind, value, string/type attrs |
| Object file classes | ~20 | Binary, Section, Symbol, Relocation |
| PassBuilderOptions | ~12 | Optimization pass configuration |
| Misc (Comdat, OperandBundle, etc.) | ~10 | Various small classes |
