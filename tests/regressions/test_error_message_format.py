#!/usr/bin/env -S uv run
"""
Regression test for bitcode parsing error message format.

The C version outputs:
    Error parsing bitcode: Unknown attribute kind (255)

The Python version outputs:
    Error parsing bitcode: Failed to parse LLVM IR:
      error: Unknown attribute kind (255)

This test documents the format difference and provides a way to verify
the fix once implemented.

Root cause: The Python binding wraps the diagnostic message differently.
The LLVMParseError exception contains "Failed to parse LLVM IR:" prefix
and the actual error is indented with "error:" prefix.

Affects lit tests:
- invalid-bitcode.test

Fix: In module_ops.py, extract just the error message from the exception
and format it to match the C version. This is a HACK for llvm-c-test
compatibility.

Expected C format:
    Error parsing bitcode: <message>
    Error with new bitcode parser: <message>

Current Python format:
    Error parsing bitcode: Failed to parse LLVM IR:
      error: <message>
"""

import llvm
from pathlib import Path


def get_parse_error_message():
    """Get the error message format from a parse failure."""
    invalid_bc = Path(__file__).parent.parent.parent / (
        "llvm-c/llvm-c-test/inputs/invalid.ll.bc"
    )

    if not invalid_bc.exists():
        print(f"Skipping test - invalid bitcode file not found: {invalid_bc}")
        return None

    with open(invalid_bc, "rb") as f:
        bitcode = f.read()

    try:
        with llvm.create_context() as ctx:
            with ctx.parse_bitcode_from_bytes(bitcode) as mod:
                pass  # Should not reach here
        return None
    except llvm.LLVMParseError as e:
        return str(e)
    except Exception as e:
        return str(e)


def test_error_message_format():
    """Document the current error message format."""
    msg = get_parse_error_message()
    if msg is None:
        print("SKIP: Could not get error message")
        return

    print(f"Current error message:\n{msg}")
    print()

    # Check what's in the message
    has_failed_prefix = "Failed to parse LLVM IR:" in msg
    has_error_prefix = "error:" in msg
    has_attribute_kind = "Unknown attribute kind" in msg

    print(f"Has 'Failed to parse LLVM IR:': {has_failed_prefix}")
    print(f"Has 'error:' prefix: {has_error_prefix}")
    print(f"Has 'Unknown attribute kind': {has_attribute_kind}")

    # Expected format for C compatibility
    expected = "Unknown attribute kind (255)"
    print(f"\nExpected core message: {expected}")
    print(f"Core message present: {expected in msg}")


def extract_error_message_hack(exception_msg: str) -> str:
    """
    HACK: Extract the core error message from LLVMParseError format.

    Input format:
        Failed to parse LLVM IR:
          error: Unknown attribute kind (255) (Producer: ...)

    Output format:
        Unknown attribute kind (255) (Producer: ...)

    This is needed for llvm-c-test compatibility.
    """
    lines = exception_msg.strip().split("\n")

    for line in lines:
        line = line.strip()
        # Find line starting with "error:" and extract the message
        if line.startswith("error:"):
            return line[len("error:") :].strip()

    # Fallback: return as-is
    return exception_msg


def test_error_message_extraction():
    """Test the HACK to extract clean error message."""
    msg = get_parse_error_message()
    if msg is None:
        print("SKIP: Could not get error message")
        return

    clean_msg = extract_error_message_hack(msg)
    print(f"Extracted message: {clean_msg}")

    # Should start with the actual error, not wrapper text
    assert not clean_msg.startswith("Failed"), (
        f"Message should not start with 'Failed': {clean_msg}"
    )
    assert not clean_msg.startswith("error:"), (
        f"Message should not start with 'error:': {clean_msg}"
    )
    assert "Unknown attribute kind" in clean_msg, (
        f"Message should contain 'Unknown attribute kind': {clean_msg}"
    )

    print("Error message extraction HACK works correctly")


if __name__ == "__main__":
    test_error_message_format()
    print()
    print("test_error_message_format: PASSED")
    print()

    test_error_message_extraction()
    print()
    print("test_error_message_extraction: PASSED")

    print("\nAll error message format tests completed!")
    print("\nNOTE: To fix invalid-bitcode.test, apply the extraction HACK")
    print("in module_ops.py to match C version output format.")
