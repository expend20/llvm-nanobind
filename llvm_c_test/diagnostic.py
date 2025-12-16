"""
llvm-c-test diagnostic handler command implementations.

This module implements the --test-diagnostic-handler command which tests
the diagnostic handler functionality.
"""

import sys
import llvm
from .helpers import create_memory_buffer_with_stdin


def test_diagnostic_handler():
    """Test diagnostic handler functionality.

    Sets up a diagnostic handler on the global context and attempts to
    load invalid bitcode to trigger it.
    """
    # Get global context
    global_ctx = llvm.global_context()

    # Set up diagnostic handler
    llvm.context_set_diagnostic_handler(global_ctx)

    # Read stdin into memory buffer
    try:
        membuf = create_memory_buffer_with_stdin()
    except Exception as e:
        print(f"Error reading file: {e}", file=sys.stderr)
        return 1

    # Try to load bitcode - this may trigger the diagnostic handler
    try:
        mod = llvm.get_bitcode_module_2(membuf)
        # If we get here, loading succeeded
    except Exception as e:
        # Loading failed, which is fine for this test
        pass

    # Check if diagnostic handler was called
    if llvm.diagnostic_was_called():
        print("Executing diagnostic handler", file=sys.stderr)

        # Get severity
        severity = llvm.get_diagnostic_severity()
        severity_name = "unknown"
        if severity == llvm.DiagnosticSeverity.Error:
            severity_name = "error"
        elif severity == llvm.DiagnosticSeverity.Warning:
            severity_name = "warning"
        elif severity == llvm.DiagnosticSeverity.Remark:
            severity_name = "remark"
        elif severity == llvm.DiagnosticSeverity.Note:
            severity_name = "note"

        print(f"Diagnostic severity is of type {severity_name}", file=sys.stderr)
        print("Diagnostic handler was called while loading module", file=sys.stderr)
    else:
        print("Diagnostic handler was not called while loading module", file=sys.stderr)

    return 0
