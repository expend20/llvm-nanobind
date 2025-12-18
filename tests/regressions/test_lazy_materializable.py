#!/usr/bin/env -S uv run
"""
Regression test for lazy module loading and materialization.

When using lazy loading (--lazy-module-dump), LLVM shows functions with
`; Materializable` comments and empty bodies `{}`.

This test verifies:
1. parse_bitcode_from_file supports lazy=True
2. parse_bitcode_from_bytes supports lazy=True
3. Lazy-loaded modules show "; Materializable" in output

The lazy parameter was added to parse_bitcode_from_bytes to fix functions.ll.

Expected output (lazy):
```
; Materializable
define i32 @X() {}
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


def test_lazy_loading_from_bytes_supported():
    """Verify parse_bitcode_from_bytes supports lazy parameter."""
    bitcode_path = Path(__file__).parent / "factorial.bc"
    with open(bitcode_path, "rb") as f:
        bitcode = f.read()

    with llvm.create_context() as ctx:
        # Test that lazy=True works
        with ctx.parse_bitcode_from_bytes(bitcode, lazy=True) as mod:
            ir = str(mod)
            has_materializable = "; Materializable" in ir
            print(f"Lazy load from bytes has '; Materializable': {has_materializable}")
            assert has_materializable, "Lazy load from bytes should show Materializable"


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

    test_lazy_loading_from_bytes_supported()
    print("test_lazy_loading_from_bytes_supported: PASSED")
    print()

    test_lazy_module_output_format()
    print("test_lazy_module_output_format: PASSED")

    print("\nAll lazy module tests passed!")
