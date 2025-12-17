#!/usr/bin/env python3
"""
Wrapper script for llvm_c_test that provides command logging.

This script is used when LLVM_C_TEST_LOG is set to log all executed commands
to a file for debugging purposes.

Environment Variables:
    LLVM_C_TEST_LOG: Path to log file for recording executed commands

Usage:
    # With logging
    LLVM_C_TEST_LOG=commands.log python llvm-c-test-wrapper.py --echo < input.bc
"""

import os
import sys
from datetime import datetime
from pathlib import Path

# Add project root to path so llvm_c_test can be imported
project_root = Path(__file__).parent.resolve()
sys.path.insert(0, str(project_root))


def log_command(log_file: str, args: list[str]) -> None:
    """Append executed command to log file."""
    timestamp = datetime.now().isoformat()
    with open(log_file, "a") as f:
        f.write(f"{timestamp} {' '.join(args)}\n")


def main() -> int:
    """Run llvm_c_test with optional logging."""
    log_file = os.environ.get("LLVM_C_TEST_LOG")

    # Log the command if logging is enabled
    if log_file:
        log_command(log_file, sys.argv)

    from llvm_c_test import main as llvm_c_test_main

    return llvm_c_test_main.main()


if __name__ == "__main__":
    sys.exit(main())
