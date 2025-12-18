#!/usr/bin/env -S uv run
"""
Regression test for lazy module loading and materialization.

When using lazy loading (--lazy-module-dump), the C version shows functions with
`; Materializable` comments and empty bodies `{}`. The Python version currently
doesn't support lazy loading from bytes - only from files.

This test documents:
1. parse_bitcode_from_file supports lazy=True
2. parse_bitcode_from_bytes does NOT support lazy (always eager)
3. Lazy-loaded modules should show "; Materializable" in output

Root cause: The module_ops.py doesn't pass lazy parameter to parse function,
and parse_bitcode_from_bytes doesn't have a lazy parameter anyway.

Affects lit tests:
- functions.ll (--lazy-module-dump command)

Fix needed:
1. Add lazy parameter to parse_bitcode_from_bytes C++ binding
2. Update module_ops.py to use lazy=True for --lazy-module-dump
3. Ensure LLVM's LLVMPrintModuleToString includes "; Materializable" for
   lazy-loaded functions (this is automatic if module is truly lazy)

Expected C output (lazy):
```
; Materializable
define i32 @X() {}
```

Current Python output (not lazy):
```
define i32 @X() {
entry:
  br label %l1
  ...
}
```
"""

import llvm
from pathlib import Path


def test_lazy_loading_from_file_supported():
    """Verify parse_bitcode_from_file supports lazy=True."""
    bitcode_path = Path(__file__).parent / "factorial.bc"

    with llvm.create_context() as ctx:
        # This should work - file API supports lazy
        with ctx.parse_bitcode_from_file(bitcode_path, lazy=True) as mod:
            # Module should load successfully
            assert mod is not None
            print(f"Lazy-loaded module from file: {mod.name}")

            # Check if functions are present
            func_names = [f.name for f in mod.functions]
            print(f"Functions: {func_names}")


def test_lazy_loading_from_bytes_not_supported():
    """Document that parse_bitcode_from_bytes doesn't have lazy parameter."""
    bitcode_path = Path(__file__).parent / "factorial.bc"
    with open(bitcode_path, "rb") as f:
        bitcode = f.read()

    with llvm.create_context() as ctx:
        # Check if lazy parameter exists
        import inspect

        sig = inspect.signature(ctx.parse_bitcode_from_bytes)
        params = list(sig.parameters.keys())

        # Currently only has 'data' parameter, no 'lazy'
        print(f"parse_bitcode_from_bytes parameters: {params}")

        has_lazy = "lazy" in params
        print(f"Has lazy parameter: {has_lazy}")

        # This is the current limitation - bytes API is always eager
        # TODO: Add lazy parameter to parse_bitcode_from_bytes
        if not has_lazy:
            print("NOTE: lazy parameter not yet supported for bytes API")


def test_lazy_module_output_format():
    """Test what lazy-loaded module output looks like.

    When truly lazy, LLVM's PrintModuleToString should include:
    - "; Materializable" comment before each function
    - Empty function bodies "{}"

    Currently this doesn't work because:
    1. parse_bitcode_from_bytes is always eager
    2. module_ops.py doesn't use lazy=True even for file API
    """
    bitcode_path = Path(__file__).parent / "factorial.bc"

    with llvm.create_context() as ctx:
        # Eager load (current behavior)
        with ctx.parse_bitcode_from_file(bitcode_path, lazy=False) as mod:
            ir_eager = str(mod)
            has_materializable_eager = "; Materializable" in ir_eager
            print(f"Eager load - has '; Materializable': {has_materializable_eager}")

        # Lazy load (should show Materializable)
        with ctx.parse_bitcode_from_file(bitcode_path, lazy=True) as mod:
            ir_lazy = str(mod)
            has_materializable_lazy = "; Materializable" in ir_lazy
            print(f"Lazy load - has '; Materializable': {has_materializable_lazy}")

            if has_materializable_lazy:
                # Find and print a Materializable line
                for line in ir_lazy.split("\n"):
                    if "Materializable" in line or line.startswith("define"):
                        print(f"  {line}")
                        if line.startswith("define"):
                            break


if __name__ == "__main__":
    test_lazy_loading_from_file_supported()
    print("test_lazy_loading_from_file_supported: PASSED")
    print()

    test_lazy_loading_from_bytes_not_supported()
    print("test_lazy_loading_from_bytes_not_supported: PASSED")
    print()

    test_lazy_module_output_format()
    print("test_lazy_module_output_format: PASSED")

    print("\nAll lazy module tests completed!")
    print("\nNOTE: To fix functions.ll test, need to:")
    print("  1. Add lazy parameter to parse_bitcode_from_bytes")
    print("  2. Update module_ops.py to use lazy=True for --lazy-module-dump")
