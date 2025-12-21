"""
Implementation of module operations commands.

Includes:
- --module-dump (and variants)
- --module-list-functions
- --module-list-globals
"""

import sys
import llvm


def _extract_error_message(exception_msg: str) -> str:
    """
    HACK: Extract the core error message from LLVMParseError format.

    The Python binding wraps errors differently than the C version.

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


def module_dump(lazy=False, new=False):
    """
    Parse bitcode from stdin and print IR.

    Args:
        lazy: If True, use lazy loading
        new: If True, use new API (diagnostic handler)
    """
    try:
        # Read stdin
        bitcode = sys.stdin.buffer.read()

        # Parse bitcode
        ctx = llvm.global_context()

        with ctx.parse_bitcode_from_bytes(bitcode, lazy=lazy) as mod:
            # HACK: When parsing from bytes, LLVM sets module ID to '<bytes>'.
            # For llvm-c-test compatibility, use '<stdin>' to match C version.
            mod.name = "<stdin>"
            # Print module IR
            print(mod.to_string(), end="")

        return 0
    except llvm.LLVMParseError as e:
        # HACK: Extract clean error message to match C version output format
        clean_msg = _extract_error_message(str(e))
        if new:
            print(f"Error with new bitcode parser: {clean_msg}", file=sys.stderr)
        else:
            print(f"Error parsing bitcode: {clean_msg}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error parsing bitcode: {e}", file=sys.stderr)
        return 1


def module_list_functions():
    """List functions in module with basic block and instruction counts."""
    try:
        # Read and parse module
        bitcode = sys.stdin.buffer.read()
        ctx = llvm.global_context()

        with ctx.parse_bitcode_from_bytes(bitcode) as mod:
            # Iterate through functions
            for func in mod.functions:
                if func.is_declaration:
                    print(f"FunctionDeclaration: {func.name}")
                else:
                    bb_count = func.basic_block_count
                    print(f"FunctionDefinition: {func.name} [#bb={bb_count}]")

                    # Count instructions and find calls
                    nisn = 0
                    nbb = 0

                    bb = func.first_basic_block
                    while bb is not None:
                        nbb += 1

                        inst = bb.first_instruction
                        while inst is not None:
                            nisn += 1

                            # Check if it's a call instruction
                            if inst.is_call_inst:
                                # Get the called function (last operand)
                                num_ops = inst.num_operands
                                if num_ops > 0:
                                    callee = inst.get_operand(num_ops - 1)
                                    print(f" calls: {callee.name}")

                            inst = inst.next_instruction

                        bb = bb.next_block

                    print(f" #isn: {nisn}")
                    print(f" #bb: {nbb}\n")

            return 0
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


def module_list_globals():
    """List global variables in module."""
    try:
        # Read and parse module
        bitcode = sys.stdin.buffer.read()
        ctx = llvm.global_context()

        with ctx.parse_bitcode_from_bytes(bitcode) as mod:
            # Iterate through globals
            for g in mod.globals:
                ty = g.type
                ty_str = str(ty)

                if g.is_declaration:
                    print(f"GlobalDeclaration: {g.name} {ty_str}")
                else:
                    print(f"GlobalDefinition: {g.name} {ty_str}")

            return 0
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
