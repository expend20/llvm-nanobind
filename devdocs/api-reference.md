# Python API Reference

This project's comprehensive API reference is the generated type stub:

- `.venv/Lib/site-packages/llvm/__init__.pyi`

That file is generated from the nanobind bindings and contains the full public
surface area (functions, classes, methods, properties, argument names, and
types) for the currently built version.

## How To Regenerate

```bash
uv sync
```

The stub is regenerated during rebuild.

## How To Browse Quickly

```bash
# List public classes
rg "^class " .venv/Lib/site-packages/llvm/__init__.pyi

# List top-level functions
rg "^def " .venv/Lib/site-packages/llvm/__init__.pyi

# Jump to one API quickly
rg "replace_all_uses_with|erase_from_parent|split_basic_block|const_string" .venv/Lib/site-packages/llvm/__init__.pyi
```

## Notes

- This is the canonical reference for exact signatures.
- Narrative usage guidance and caveats remain in:
  - `README.md`
  - `devdocs/porting-guide.md`
  - `devdocs/lit-tests.md`
  - `devdocs/DEBUGGING.md`
