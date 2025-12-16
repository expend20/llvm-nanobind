"""
llvm-c-test debug info command implementations.

This module implements debug info related commands like --get-di-tag and
--di-type-get-name.
"""

import llvm


def get_di_tag():
    """Test LLVMGetDINodeTag functionality.

    This is a silent test that creates debug info nodes and queries their tags.
    Returns 0 on success.
    """
    # Create module with global context
    ctx = llvm.global_context()
    with ctx.create_module("Mod") as mod:
        # Create metadata string and node
        string_md = llvm.md_string_in_context_2(ctx, "foo")
        node_md = llvm.md_node_in_context_2(ctx, [string_md])

        # Get tag from node (should be 0 for generic node)
        tag = llvm.get_di_node_tag(node_md)
        assert tag == 0, f"Expected tag 0, got {tag}"

        # Create DIBuilder
        dib = llvm.create_dibuilder(mod)

        # Create file
        file_md = llvm.dibuilder_create_file(dib, "metadata.c", ".")

        # Create struct type (DW_TAG_structure_type = 0x13)
        struct_md = llvm.dibuilder_create_struct_type(
            dib,
            file_md,
            "TestClass",
            file_md,
            42,  # line number
            64,  # size in bits
            0,  # align in bits
            llvm.DIFlagObjcClassComplete,  # flags
        )

        # Get tag from struct (should be 0x13 = DW_TAG_structure_type)
        tag = llvm.get_di_node_tag(struct_md)
        assert tag == 0x13, f"Expected tag 0x13, got {tag:#x}"

    return 0


def di_type_get_name():
    """Test LLVMDITypeGetName functionality.

    This is a silent test that creates a debug info type and queries its name.
    Returns 0 on success.
    """
    # Create module with global context
    ctx = llvm.global_context()
    with ctx.create_module("Mod") as mod:
        # Create DIBuilder
        dib = llvm.create_dibuilder(mod)

        # Create file
        file_md = llvm.dibuilder_create_file(dib, "metadata.c", ".")

        # Create struct type with name
        name = "TestClass"
        struct_md = llvm.dibuilder_create_struct_type(
            dib,
            file_md,
            name,
            file_md,
            42,  # line number
            64,  # size in bits
            0,  # align in bits
            llvm.DIFlagObjcClassComplete,  # flags
        )

        # Get name from type
        type_name = llvm.di_type_get_name(struct_md)

        # Verify name matches
        assert len(type_name) == len(name), (
            f"Expected length {len(name)}, got {len(type_name)}"
        )
        assert type_name == name, f"Expected name '{name}', got '{type_name}'"

    return 0
