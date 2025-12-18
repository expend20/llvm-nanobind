#!/usr/bin/env -S uv run
"""
Regression test for module ID mismatch when parsing from bytes.

When parsing bitcode from bytes (e.g., reading stdin), the module ID is set to
'<bytes>' but the LLVM C test expects '<stdin>' for compatibility with the C
version which reads directly from stdin.

This test verifies that:
1. Parsing from bytes sets module ID to '<bytes>' (current behavior)
2. The module name can be changed after parsing to match expected output

Root cause: The parse_bitcode_from_bytes API was introduced to replace the
stdin-specific parsing function. The module ID comes from LLVM based on how
the bitcode was provided.

Affects lit tests:
- atomics.ll
- empty.ll
- float_ops.ll
- freeze.ll
- invoke.ll
- memops.ll

Fix: In echo.py, rename the module to '<stdin>' after parsing when reading
from stdin to match the C version's behavior.
"""

import llvm
from pathlib import Path


def test_module_id_from_bytes():
    """Verify module ID is '<bytes>' when parsing from bytes."""
    # Load test bitcode
    bitcode_path = Path(__file__).parent / "factorial.bc"
    with open(bitcode_path, "rb") as f:
        bitcode = f.read()

    with llvm.create_context() as ctx:
        with ctx.parse_bitcode_from_bytes(bitcode) as mod:
            # Current behavior: module ID is '<bytes>'
            assert mod.name == "<bytes>", f"Expected '<bytes>', got '{mod.name}'"
            print(f"Module name after parsing from bytes: {mod.name}")


def test_module_id_can_be_renamed():
    """Verify module name can be changed to '<stdin>' after parsing."""
    bitcode_path = Path(__file__).parent / "factorial.bc"
    with open(bitcode_path, "rb") as f:
        bitcode = f.read()

    with llvm.create_context() as ctx:
        with ctx.parse_bitcode_from_bytes(bitcode) as mod:
            # Rename to match expected C behavior
            mod.name = "<stdin>"
            assert mod.name == "<stdin>", f"Expected '<stdin>', got '{mod.name}'"

            # Verify it appears in the IR output
            ir = str(mod)
            assert "; ModuleID = '<stdin>'" in ir, (
                f"Expected ModuleID to be '<stdin>' in IR output"
            )
            print("Module successfully renamed to '<stdin>'")


def test_module_id_in_ir_output():
    """Verify the module ID appears correctly in IR output."""
    bitcode_path = Path(__file__).parent / "factorial.bc"
    with open(bitcode_path, "rb") as f:
        bitcode = f.read()

    with llvm.create_context() as ctx:
        with ctx.parse_bitcode_from_bytes(bitcode) as mod:
            ir_before = str(mod)
            assert "; ModuleID = '<bytes>'" in ir_before, (
                "Expected '<bytes>' in IR before rename"
            )

            mod.name = "<stdin>"
            ir_after = str(mod)
            assert "; ModuleID = '<stdin>'" in ir_after, (
                "Expected '<stdin>' in IR after rename"
            )
            print("IR output correctly reflects module name changes")


if __name__ == "__main__":
    test_module_id_from_bytes()
    print("test_module_id_from_bytes: PASSED")

    test_module_id_can_be_renamed()
    print("test_module_id_can_be_renamed: PASSED")

    test_module_id_in_ir_output()
    print("test_module_id_in_ir_output: PASSED")

    print("\nAll module ID tests passed!")
